
// Default LUT binary.
let default_lut_rom = new Uint8Array([
    0x80, 0x66, 0x96, 0x51, 0x40, 0x04, 0x00,
    0x00, 0x00, 0x00, 0x10, 0x66, 0x96, 0x88,
    0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x8A,
    0x66, 0x96, 0x51, 0x0B, 0x2F, 0x00, 0x00,
    0x00, 0x00, 0x8A, 0x66, 0x96, 0x51, 0x0B,
    
    0x2F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x5A, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x2D, 0x55, 0x28, 0x25, 0x00,
    0x02, 0x03, 0x01, 0x02, 0x0C,
    0x12, 0x01, 0x12, 0x01, 0x03,
    0x05, 0x05, 0x02, 0x05, 0x02,
    
    0x07, 0x01, 0x07, 0x2A, 0x04, 0x04,
]);

// Phase names.
let phase_name    = ["A", "B", "C", "D"];
// LUT color level names.
let lut_col_name  = ["VSS", "VSH1", "VSL", "VSH2"];
// LUT VCOM level names.
let lut_vcom_name = ["VCOM", "VCOM+VSH1", "VCOM+VSL", "VCOM+VSH2"];

// Current LUT data.
var cur_lut;


function getChildrenRecursively(parent) {
    var children = Array.from(parent.children);
    for (var i = 0; i < parent.children.length; i++) {
        children = children.concat(getChildrenRecursively(parent.children[i]));
    }
    return children;
}

function loaded() {
    cur_lut = unpackLUT(default_lut_rom);
    genTable();
    getChildrenRecursively(document.getElementById("editor")).forEach(e => e.onchange = () => {readTable(); updateBytes();});
    updateTable();
    updateBytes();
}



// Generate the LUT editor table.
function genTable() {
    let parent = document.getElementById("wftable");
    
    function addOptions(select, names) {
        for (var i = 0; i < names.length; i++) {
            var option       = document.createElement("option");
            option.value     = i;
            option.innerText = names[i];
            select.appendChild(option);
        }
    }
    
    for (var x = 0; x < 7; x++) {
        var tr0 = document.createElement("tr");
        var tr0td0 = document.createElement("td");
        var tr0td1 = document.createElement("td");
        
        var t1     = document.createElement("table");
        var tr1    = document.createElement("tr");
        var tr1td0 = document.createElement("td");
        var tr1td1 = document.createElement("td");
        var repeat = document.createElement("input");
        
        tr0td0.innerHTML = `<p>${x}</p>`;
        for (var y = 0; y < 4; y++) {
            var phtr  = document.createElement("tr");
            var phtd0 = document.createElement("td");
            var phtd1 = document.createElement("td");
            var phtd2 = document.createElement("td");
            var phtd3 = document.createElement("td");
            var phtd4 = document.createElement("td");
            var phtd5 = document.createElement("td");
            
            var input = document.createElement("input");
            var sel0  = document.createElement("select");
            var sel1  = document.createElement("select");
            var sel2  = document.createElement("select");
            var sel3  = document.createElement("select");
            
            phtr.setAttribute("class", "wf");
            phtd0.innerHTML = `<p>${phase_name[y]}:</p>`;
            input.setAttribute("id", `group_${x}_phase_${y}_l`);
            input.setAttribute("type", "number");
            sel0.setAttribute("id", `group_${x}_phase_${y}_b`);
            sel1.setAttribute("id", `group_${x}_phase_${y}_w`);
            sel2.setAttribute("id", `group_${x}_phase_${y}_r`);
            sel3.setAttribute("id", `group_${x}_phase_${y}_c`);
            phtd2.setAttribute("class", "b");
            phtd3.setAttribute("class", "w");
            phtd4.setAttribute("class", "r");
            phtd5.setAttribute("class", "c");
            addOptions(sel0, lut_col_name);
            addOptions(sel1, lut_col_name);
            addOptions(sel2, lut_col_name);
            addOptions(sel3, lut_vcom_name);
            
            phtd1.appendChild(input);
            phtd2.appendChild(sel0);
            phtd3.appendChild(sel1);
            phtd4.appendChild(sel2);
            phtd5.appendChild(sel3);
            
            phtr.appendChild(phtd0);
            phtr.appendChild(phtd1);
            phtr.appendChild(phtd2);
            phtr.appendChild(phtd3);
            phtr.appendChild(phtd4);
            phtr.appendChild(phtd5);
            t1.appendChild(phtr);
        }
        tr1td0.innerHTML = "<p>Repeat:</p>";
        repeat.setAttribute("id", `group_${x}_r`);
        repeat.setAttribute("type", "number");
        tr1td1.appendChild(repeat);
        
        tr1.appendChild(tr1td0);
        tr1.appendChild(tr1td1);
        t1.appendChild(tr1);
        tr0td1.appendChild(t1);
        
        tr0.appendChild(tr0td0);
        tr0.appendChild(tr0td1);
        parent.appendChild(tr0);
    }
}

// Update the LUT editor table.
function updateTable() {
    for (var x = 0; x < 7; x++) {
        document.getElementById(`group_${x}_r`).value = cur_lut.groups[x].repeat;
        for (var y = 0; y < 4; y++) {
            document.getElementById(`group_${x}_phase_${y}_b`).value = cur_lut.groups[x].phases[y].black;
            document.getElementById(`group_${x}_phase_${y}_w`).value = cur_lut.groups[x].phases[y].white;
            document.getElementById(`group_${x}_phase_${y}_r`).value = cur_lut.groups[x].phases[y].red;
            document.getElementById(`group_${x}_phase_${y}_c`).value = cur_lut.groups[x].phases[y].vcom;
            document.getElementById(`group_${x}_phase_${y}_l`).value = cur_lut.groups[x].phases[y].length;
        }
    }
    // document.getElementById("gate_level").value = cur_lut.gate_level;
    // document.getElementById("vsh1_level").value = cur_lut.vsh1_level;
    // document.getElementById("vsh2_level").value = cur_lut.vsh2_level;
    // document.getElementById("vsl_level").value  = cur_lut.vsl_level;
    // document.getElementById("frame1").value     = cur_lut.frame1;
    // document.getElementById("frame2").value     = cur_lut.frame2;
}

// Read the LUT editor table.
function readTable() {
    for (var x = 0; x < 7; x++) {
        cur_lut.groups[x].repeat = Number(document.getElementById(`group_${x}_r`).value);
        for (var y = 0; y < 4; y++) {
            cur_lut.groups[x].phases[y].black  = Number(document.getElementById(`group_${x}_phase_${y}_b`).value);
            cur_lut.groups[x].phases[y].white  = Number(document.getElementById(`group_${x}_phase_${y}_w`).value);
            cur_lut.groups[x].phases[y].red    = Number(document.getElementById(`group_${x}_phase_${y}_r`).value);
            cur_lut.groups[x].phases[y].vcom   = Number(document.getElementById(`group_${x}_phase_${y}_c`).value);
            cur_lut.groups[x].phases[y].length = Number(document.getElementById(`group_${x}_phase_${y}_l`).value);
        }
    }
    // cur_lut.gate_level = Number(document.getElementById("gate_level").value);
    // cur_lut.vsh1_level = Number(document.getElementById("vsh1_level").value);
    // cur_lut.vsh2_level = Number(document.getElementById("vsh2_level").value);
    // cur_lut.vsl_level  = Number(document.getElementById("vsl_level").value);
    // cur_lut.frame1     = Number(document.getElementById("frame1").value);
    // cur_lut.frame2     = Number(document.getElementById("frame2").value);
}



// Update the raw bytes.
function updateBytes() {
    let raw = packLUT(cur_lut);
    var bytes = document.getElementById("bytes");
    var str   = "";
    for (var i = 0; i < raw.length; i++) {
        if (i && i % 16 == 0) str += '\n';
        str += `0x${raw[i].toString(16).padStart(2, "0")},`;
    }
    bytes.value = str.substring(0, str.length-1);
}

// Read the raw bytes.
function readBytes() {
    // TODO.
}



// Unpack a LUT binary.
function unpackLUT(raw) {
    var groups = [{}, {}, {}, {}, {}, {}, {}];
    
    function readVS(group, phase, index) {
        let byte = raw[group + index*7];
        return (byte >> (6-phase*2)) & 3;
    }
    
    for (var i = 0; i < 7; i++) {
        groups[i] = {
            phases: [{}, {}, {}, {}],
            repeat: raw[39+5*i],
        };
        for (var x = 0; x < 4; x++) {
            groups[i].phases[x] = {
                black:  readVS(i, x, 0),
                white:  readVS(i, x, 1),
                red:    readVS(i, x, 2),
                vcom:   readVS(i, x, 4),
                length: raw[35+5*i+x]
            };
        }
    }
    
    return {
        groups:     groups,
        // gate_level: raw[70],
        // vsh1_level: raw[71],
        // vsh2_level: raw[72],
        // vsl_level:  raw[73],
        // frame1:     raw[74],
        // frame2:     raw[75],
    };
}

// Pack a LUT binary.
function packLUT(lut) {
    var raw = new Uint8Array(70);
    
    function writeVS(group, phase, index, value) {
        raw[group + index*7] &= ~(3 << (6-phase*2));
        raw[group + index*7] |= (value & 3) << (6-phase*2);
    }
    
    for (var i = 0; i < 7; i++) {
        raw[39+5*i] = lut.groups[i].repeat;
        for (var x = 0; x < 4; x++) {
            writeVS(i, x, 0, lut.groups[i].phases[x].black);
            writeVS(i, x, 1, lut.groups[i].phases[x].white);
            writeVS(i, x, 2, lut.groups[i].phases[x].red);
            writeVS(i, x, 3, lut.groups[i].phases[x].red);
            writeVS(i, x, 4, lut.groups[i].phases[x].vcom);
            raw[35+5*i+x] = lut.groups[i].phases[x].length;
        }
    }
    
    // raw[70] = lut.gate_level;
    // raw[71] = lut.vsh1_level;
    // raw[72] = lut.vsh2_level;
    // raw[73] = lut.vsl_level;
    // raw[74] = lut.frame1;
    // raw[75] = lut.frame2;
    return raw;
}
