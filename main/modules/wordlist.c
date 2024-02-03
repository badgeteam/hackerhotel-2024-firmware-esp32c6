#include "application.h"
#include "badge_comms.h"
#include "badge_messages.h"
#include "bsp.h"
#include "esp_err.h"
#include "esp_log.h"
#include "events.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "pax_codecs.h"
#include "pax_gfx.h"
#include "screen_home.h"
#include "screen_pointclick.h"
#include "screen_repertoire.h"
#include "screens.h"
#include "textedit.h"
#include <inttypes.h>
#include <stdio.h>
#include <esp_random.h>
#include <string.h>


// static char const * TAG = "wordlist";

// static char const * const victorian_words[] = {
//     "Abode",
//     "Absquatulate",
//     "Afeared",
//     "Afford",
//     "Alight",
//     "Artificer",
//     "Asunder",
//     "Balderdash",
//     "Blazes",
//     "Boarder",
//     "Bog",
//     "Boodle",
//     "Boot",
//     "Brake",
//     "Bricky",
//     "Brig",
//     "Bronchitis",
//     "Cane",
//     "Cavort",
//     "Cesspool",
//     "Chilblains",
//     "Chinwag",
//     "Chirk",
//     "Cholera",
//     "Chuckaboo",
//     "Coastguard",
//     "Commencement",
//     "Constitution",
//     "Constitutional",
//     "Convalescent",
//     "Conveyed",
//     "Coot",
//     "Corned",
//     "Countenance",
//     "Cursory",
//     "Daddles",
//     "Dashing",
//     "Diphtheria",
//     "Dodgy",
//     "Doggery",
//     "Dratted",
//     "Drest",
//     "Drumsticks",
//     "Empress",
//     "Encumbered",
//     "Enfeebled",
//     "Epidemic",
//     "Establishment",
//     "Exfluncticate",
//     "Fled",
//     "Foozler",
//     "Frequented",
//     "Furtherance",
//     "Gallery",
//     "Getter",
//     "Gig",
//     "Gigglemug",
//     "Gruel",
//     "Gutted",
//     "Hopscotch",
//     "Hulk",
//     "Inkwell",
//     "Interred",
//     "Invalid",
//     "Journeyman",
//     "Junk",
//     "Kick",
//     "Knackered",
//     "Laden",
//     "Laundress",
//     "Liberal",
//     "Locomotive",
//     "Lodger",
//     "Logbook",
//     "Malt",
//     "Maltster",
//     "Maypole",
//     "Meater",
//     "Miasma",
//     "Miasmatic",
//     "Mollisher",
//     "Mortality",
//     "Mosey",
//     "Muffler",
//     "Nanny",
//     "Notwithstanding",
//     "Ocular",
//     "Omnibus",
//     "Opine",
//     "Ozone",
//     "Page",
//     "Parish",
//     "Parlour",
//     "Peart",
//     "Peelers",
//     "Pertaining",
//     "Phonograph",
//     "Piecer",
//     "Pneumonia",
//     "Polyphon",
//     "Poorhouse",
//     "Posh",
//     "Postboy",
//     "Posting",
//     "Precinct",
//     "Proper",
//     "Proprietor",
//     "Prosperity",
//     "Ramstuginous",
//     "Reformer",
//     "Rubbish",
//     "Ruin",
//     "Sagacious",
//     "Salubrious",
//     "Shilling",
//     "Slate",
//     "Slum",
//     "Smack",
//     "Solicited",
//     "Stagecoach",
//     "Stricken",
//     "Suggestionize",
//     "Tarnation",
//     "Tradesman",
//     "Tram",
//     "Transportation",
//     "Trappers",
//     "Treadmill",
//     "Trunk",
//     "Turnpike",
//     "Typhoid",
//     "Unions",
//     "Vamose",
//     "Vapour",
//     "Wireless",
//     "Withal",
//     "Workhouse"
// };
// static char const * const victorian_definitions[] = {
//     "Home, a place where you live. ",
//     "To take leave, to disappear.",
//     "Scared, frightened.",
//     "To provide or allow. ",
//     "Get onto or down from a train/carriage/bus.",
//     "Tradesman.",
//     "Apart.",
//     "Nonsense; foolishness; empty babble.",
//     "Used as a Victorian swear word, this slang term could mean either hell or the Devil.",
//     "A person who lives in someone else's house and pays rent for a room and meals. ",
//     "Though a “bog” can refer to a kind of swamp, it usually means “toilet” in British English. The word evolved from
//     " "“bog-house” (a kind of outdoor bathroom), which itself derived from the Old English term, “boggard.”", "A
//     crowd of people", "The word “boot” refers to the trunk or rear compartment of a car. In the time before cars,
//     carriage drivers had " "to store their boots and tools in a special compartment under their feet.", "An
//     open-topped horse drawn vehicle with four wheels which carry up to six people. ", " This descriptive slang word
//     indicates that someone has a brave nature. A bricky person isn't afraid of anything.", "A two-masted sailing
//     ship. ", "A chest infection which causes coughing and breathing problems.", "A stick used by teachers to beat
//     naughty children. ", " To frolic or prance about.", "A pit in the ground where sewage and drainage from houses
//     collects.", "An itchy swelling on the body, particularly face, feet, hands, fingers and toes cause by being
//     constantly cold or " "damp. ", "A “chinwag” is just a casual conversation. Your chin moves when you speak, and
//     the word “wag” is a synonym for " "“shake.”", "Cheerful. Synonyms: chirp, chirpy.", "A water-borne disease which
//     caused many deaths in the Victorian era. ", "This descriptive term is used to affectionately describe someone who
//     is a good friend.", "Members of the armed forces, posted along the coast to save lives at sea, the name persists
//     to today. ", "Start, beginning.", "A person's physical strengths and weaknesses. ", "A daily walk aiming to
//     improve the constitution. ", "A person recovering from an illness. ", "Carried.", "An idiot; a simpleton; a
//     ninny.", "Drunk.", "Face.", "Quick and not thorough. ", "This slang refers to the most important tool of any
//     writer's trade — their hands.", "Showy, elegant or spirited, especially in dress.", "A bacterial infection,
//     usually of the nose and throat, which is highly contagious. Common in Victorian times and " "caused many
//     deaths.", "Is very similar to the phrase “sketchy” in American English. Both terms refer to something that is
//     wrong, " "illegal, or somehow disingenuous.", "A cheap drinking establishment; in modern lingo, a dive. ", "This
//     mild Victorian swear word has the same meaning as damn in modern times. It was used as an expletive.", "Another
//     spelling of dressed.", "This word was used as a slang expression for a person's legs.", "Female ruler of an
//     empire, Queen Victoria was also an empress. ", "To be restricted in movement, 'He was encumbered by the heavy bag
//     he carried.' ", "To be made weak. ", "When an illness spreads through a population until many people are sick
//     with it. ", "A shop or a place of business. ", "To utterly destroy.", "To have run away", "This term refers to
//     someone who tends to mess things up, such as one who is clumsy in a way that causes items to " "get damaged.",
//     "Visited often.",
//     "To help or increase. ",
//     "The seam of coal in a mine which was being worked. ",
//     "A coal miner who breaks the coal down to separate it from other rock. ",
//     "A two wheeled carriage drawn by one horse. Suitable for two-three people. ",
//     "This term is used to describe someone who constantly has a smile on their face.",
//     "A thin, watery porridge. A cheap food often found in workhouses and in poor homes. ",
//     "Refers to strong negative emotions. The closest synonyms are “devastated” or “grief-stricken.” ",
//     "A jumping game played on squares drawn onto the street or playground. ",
//     "Prison ship anchored near the coast. They usually held prisoners waiting to be transported to the colonies. ",
//     "A hole in a desk to hold an ink bottle. Pens had to be dipped in ink every so often. ",
//     "Buried",
//     "Someone who is very poorly and may be unable to walk or even get out of bed. ",
//     "A skilled tradesman such as a painter, carpenter or plumber.",
//     "A Chinese sailing ship.",
//     "To protest or to object to something; to complain.",
//     "Means “extremely tired.” This term comes from the verb “knacker,” which means “to make tired” or “tire out.” "
//     "However, the term once referred to people who bought worn-out animals.",
//     "Loaded.",
//     "A woman who works from home doing other people's washing. ",
//     "Generous.",
//     "Steam train.",
//     "A person who rents a room in someone else's house. ",
//     "A record of attendance, behaviour and learning filled in by a teacher about their pupils",
//     "Sprouted barley, used for brewing beer. ",
//     "A person who makes and sells malt. ",
//     "A tall pole with long ribbons attached to the top. Used on May day for dancing - each dancer holds the end of a
//     " "ribbon and wraps it around the pole as they dance. ", "The descriptive term meater was the slang antonym for
//     bricky. This slang would have been used to describe someone " "who has a cowardly nature.", "Bad air, often
//     indicated by a bad smell, thought to spread disease. ", "The theory that disease was caused by bad air and
//     smells. ", "This is the female romantic companion of a villain, criminal or gangster.", "The number of people
//     dying, the death rate.", "To saunter or shuffle along.", "Scarf.", "A female servant who would care for the
//     children of a rich family.", "Nevertheless.", "To do with the eyes. An ocularist would be an expert in eyes. ",
//     "A bus, which could be single or double decker and were pulled by horses. ",
//     "To be of the opinion.",
//     "A gas naturally present in the air, particularly at the coast. Victorians believed that 'sea air' was an "
//     "effective remedy for many ailments. ",
//     "A servant boy.",
//     "In Victorian times, each district was split into parishes which were administrative areas. Each parish was "
//     "responsible for its own poor. ",
//     "Living room or lounge. The parlour was normally kept 'for best'. ",
//     "Fresh and happy, sprightly.",
//     "Policemen. Named after Robert Peel who introduced them. ",
//     "To do with, concerning. ",
//     "Invented in 1877 by Thomas Eddison, this was an early type of music player. ",
//     "A mill worker who joined pieces of thread together. This job was usually done by a child.",
//     "A lung infection with symptoms similar to pleurisy. ",
//     "A wooden music box which played bell-like music. It used flat metal discs punched with holes. The holes moved "
//     "over 'teeth' which were connected to tiny bells. ",
//     "Another name for the workhouse, where people too sick, old or poor to fend for themselves had to go in order to
//     " "survive. ", "An adjective to describe someone or something from the upper classes. It is usually seen as a
//     derogatory term. " "Though the origins of the word are disputed, many believe that “posh” is an acronym of the
//     phrase “Port Out, " "Starboard Home.” This phrase refers to the best sides of the ship chosen by rich passengers
//     (portside on the " "initial journey and starboard on the return trip).", "A servant who would work at an inn, or
//     post house, and would change the horses on the carriages for travellers. ", "Travelling by carriage and stopping
//     at post houses at intervals to change the horses. ", "A specific area. ", "In most forms of English, “proper”
//     refers to the “correct” way of doing something. However, in British slang, " "“proper” is frequently used in
//     place of “very.", "The owner of a shop or business. ", "Money and success.", "Rowdy, disorderly or boisterous.",
//     "A person who works and campaigns to make living conditions better in all sorts of areas, but especially for the
//     " "poor. ", "Though “rubbish” literally means “garbage” or “trash,” it can also be used as a criticism. Calling
//     something " "“rubbish” is much like calling it “ridiculous” or “nonsensical.” ", "To go out of business and have
//     no money to live on. ", "Wise.", "Healthy.", "An old coin which was worth 12 old pence, or five pence in today's
//     money. ", "A small piece of slate (grey, flat stone), about the size of a mini whiteboard and sometimes with a
//     wooden frame, " "that children could write on at school. ", "A poor area with overcrowded housing and no
//     sanitation. Often associated with crime. ", "A fishing boat.", "Requested or asked for.", "A four wheeled horse
//     drawn carriage which travelled a fixed route and stopped at the same places along the way. A " "bit like a long
//     distance bus", "Afflicted by, suffering from.", "This slang word means pretty much what it sounds like, as it
//     would be used to indicate that someone is making a " "suggestion.", "This slang term evolved as a Victorian
//     euphemism to say instead of using the curse word damnation as an " "exclamation.", "There are two types of
//     tradesman; a shopkeeper or someone who buys and sells to shopkeepers and a skilled workman " "such as a plumber
//     or a joiner. ", "A public transport vehicle that travels on rails, similar to a bus or train. Originally pulled
//     by horses but " "later powered by a steam engine. ", "A punishment for criminals. They were sent to prison
//     colonies in Australia. The last cargo of prisoners arrived " "in Australia in 1868. ", "Coal mines had a system
//     of doors which opened and closed to allow air flow and to stop a build up of toxic gases. " "Trappers were
//     children, as young as five, who were employed to open and shut these trap doors. ", "A large wheel with step in
//     it. Prisoners had to stand at the wheel and turn it with their legs. The wheel did " "nothing, it was just to
//     keep the prisoners busy and keep them too tired to be any trouble. ", "A large box for carrying clothes and so
//     forth when travelling. Equivalent to a suitcase today. ", "A road people had to pay to travel on. The toll money
//     was used for the upkeep of the road.", "A bacterial infection which causes red spots on the abdomen and bowel
//     irritation. It is caused by poor sewage and " "drainage. Prince Albert died of typhoid. ", "Parishes had to band
//     together after the 1834 poor law in order to build workhouses. A group of parishes was known " "as a union. And
//     their workhouses as union workhouses which Scrooge mentions in A Christmas Carol. ", "To disappear or leave
//     quickly", "Steam, fumes, mists.", "Another name for a radio. Wireless telegraphy was invented in 1896 by Marconi
//     and allowed people to communicate " "by radio. ", "In addition, as well.", "An institution set up to look after
//     the poor, disabled or old who cannot sustain themselves. Conditions were " "deliberately below what the lowest
//     paid labourer could expect in order to 'encourage' poor people to work. People " "in the workhouse often lived in
//     harsh conditions and many people would rather die than go there. More were built " "after the 1834 Poor Law. "
// };
// // static char const * const english_words[] = {"linemate", "deraumere", "sibur", "mendiane", "phiras", "thystane"};

// // void GetVictorianWord(char const * word) {

// void GetVictorianWord(void) {
//     ESP_LOGE(TAG, "sizeof %d", sizeof(victorian_words));
// }