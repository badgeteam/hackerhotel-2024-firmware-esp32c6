# bodge comms

This component is a framework to broadcast structs over ieee802154.
This component wraps the communication to a pub-sub like model.

## working with bodge comms

### message types

a message type is like the 'topic' of a pub-sub model, which identifies the contents should be read.
these message types are defined in the struct `badge_comms_message_type_t`, which is found in `badge_messages.h`.

### listeners

a listener is like a 'subscription' in a pub-sub model.

add a listener by calling `badge_comms_add_listener`, this function needs two things: the message type and the size of the struct the data will be cast to.
the add listener function returns (if successful) a handle to a queue.
bodge comms will put relevant received messages on this queue.

when a listener is no longer in use, it should be removed by calling `badge_comms_remove_listener`.
note: this free's the queue from memory, meaning the handle is no longer usable.
we want to remove unused listeners, because there is a maximum mount of listeners we can have.

### sending a message

to 'publish' a message we call `badge_comms_send_message`.
this function broadcasts a message to all nearby bodges.

## adding a new message type

a new feature might require a new message type.
to add a message type goto `badge_comms_message_type_t` in `badge_messages.h`, and add a new message.
the message type should be assigned a number, this to make it explicitly clear what the message identifier are.

now that we have a message type, we need a struct to define how and what data is sent.

the ieee802154 protocol has a limited length.
to make sure we do not exceed this limit we add a static assert to check the size of the struct against BADGE_COMMS_USER_DATA_MAX_LEN.

## ieee802154 message layout

| bytes             | data type                     |
|-------------------|-------------------------------|
| 0                 | message size ieee802154       |
| 2 & 3             | unknown                       |
| 4 & 5             | channel (hardcoded at 0x1234) |
| 6 & 7             | to short mac address          |
| 8 - 16            | sender mac                    |
| message dependant | user data                     |
| second to last    | rssi                          |
| last byte, lsB    | lqi (whatever it means)       |
| last byte, msB    | unknown                       |
