
// Default LUT binary.
let default_lut_rom = new Uint8Array([
0x82, 0x66, 0x96, 0x51, 0x40, 0x04, 0x00, 0x00, 0x00, 0x00, 0x11, 0x66, 0x96, 0xa8, 0x20, 0x20,
0x00, 0x00, 0x00, 0x00, 0x8a, 0x66, 0x96, 0x91, 0x2b, 0x2f, 0x00, 0x00, 0x00, 0x00, 0x8a, 0x66,
0x96, 0x91, 0x2b, 0x2f, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x5a, 0x60, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x46, 0x7a, 0x75, 0x0c, 0x00, 0x02, 0x03, 0x01, 0x02, 0x0e, 0x12, 0x01, 0x12, 0x01,
0x04, 0x04, 0x0a, 0x06, 0x08, 0x02, 0x06, 0x04, 0x02, 0x2e, 0x04, 0x14, 0x06, 0x02, 0x2a, 0x04,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x2a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x13, 0x3c, 0xc1, 0x2e, 0x50, 0x11, 0x0d, 0x01, 0x01, 0x01, 0x01, 0x01,
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01
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

function load_default() {
    cur_lut = unpackLUT(default_lut_rom, 10);
    genTable();
    getChildrenRecursively(document.getElementById("editor")).forEach(e => e.onchange = () => {readTable(); updateBytes();});
    updateTable();
    updateBytes();
}

function load(arg) {
    cur_lut = unpackLUT(arg, 10);
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
    
    for (var x = 0; x < 10; x++) {
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
    for (var x = 0; x < 10; x++) {
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
    for (var x = 0; x < 10; x++) {
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

function concatTypedArrays(a, b) { // a, b TypedArray of same type
    var c = new (a.constructor)(a.length + b.length);
    c.set(a, 0);
    c.set(b, a.length);
    return c;
}

// Update the raw bytes.
function updateBytes() {
    let raw = packLUT(cur_lut, 10);
    if (raw.length < default_lut_rom.length) {
        raw = concatTypedArrays(raw, default_lut_rom.slice(raw.length));
    }
    var bytes = document.getElementById("bytes");
    var str   = "";
    for (var i = 0; i < raw.length; i++) {
        if (i && i % 16 == 0) {
            str += '\n';
        } else if (i) {
            str += ' ';
        }
        str += `0x${raw[i].toString(16).padStart(2, "0")},`;
    }
    bytes.value = str.substring(0, str.length-1);
}

// Read the raw bytes.
function readBytes() {
    // TODO.
}

// Unpack a LUT binary.
function unpackLUT(raw, width) {
    let lutSize   = width * 5;
    let groupSize = width * 5;
    let totalSize = lutSize + groupSize;

    function readVS(group, phase, index) {
        let byte = raw[group + index*width];
        return (byte >> (6-phase*2)) & 3;
    }
    
    var groups = [];
    for (var i = 0; i < width; i++) {
        groups[i] = {
            phases: [{}, {}, {}, {}],
            repeat: raw[lutSize + 5*i + 4],
        };
        for (var x = 0; x < 4; x++) {
            groups[i].phases[x] = {
                black:  readVS(i, x, 0),
                white:  readVS(i, x, 1),
                red:    readVS(i, x, 2),
                vcom:   readVS(i, x, 4),
                length: raw[lutSize + 5*i + x]
            };
        }
    }
    
    return {
        groups:     groups,
        gate_level: raw[totalSize + 0],
        vsh1_level: raw[totalSize + 1],
        vsh2_level: raw[totalSize + 2],
        vsl_level:  raw[totalSize + 3],
        frame1:     raw[totalSize + 4],
        frame2:     raw[totalSize + 5],
    };
}

// Pack a LUT binary.
function packLUT(lut, width) {
    let lutSize   = width * 5;
    let groupSize = width * 5;
    let totalSize = lutSize + groupSize;
    var raw = new Uint8Array(totalSize);
    
    function writeVS(group, phase, index, value) {
        raw[group + index*width] &= ~(3 << (6-phase*2));
        raw[group + index*width] |= (value & 3) << (6-phase*2);
    }
    
    for (var i = 0; i < width; i++) {
        raw[lutSize + 5*i + 4] = lut.groups[i].repeat;
        for (var x = 0; x < 4; x++) {
            writeVS(i, x, 0, lut.groups[i].phases[x].black);
            writeVS(i, x, 1, lut.groups[i].phases[x].white);
            writeVS(i, x, 2, lut.groups[i].phases[x].red);
            writeVS(i, x, 3, lut.groups[i].phases[x].red);
            writeVS(i, x, 4, lut.groups[i].phases[x].vcom);
            raw[lutSize + 5*i + x] = lut.groups[i].phases[x].length;
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
