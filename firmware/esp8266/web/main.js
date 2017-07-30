function sendParameter(dict) {
    const xhr = new XMLHttpRequest();

    xhr.open('POST', '/parameters');
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.onload = () => {};
    xhr.send(JSON.stringify(dict));
}

function attachEventHandlers() {
    const colorwheel = document.getElementById('colorwheel');
    const brightness = document.getElementById('brightness');
    const lowPower = document.getElementById('lowPower');
    const highPower = document.getElementById('highPower');
    const white = document.getElementById('white');
    const cinema = document.getElementById('cinema');
    const mood = document.getElementById('mood');

    brightness.addEventListener('input', () => {
        sendParameter({ brightness: parseFloat(brightness.value) });
    });
    lowPower.addEventListener('input', () => {
        sendParameter({ lowPower: parseFloat(lowPower.value) });
    });
    highPower.addEventListener('input', () => {
        sendParameter({ highPower: parseFloat(highPower.value) });
    });

    white.addEventListener('change', () => {
        sendParameter({ mode: "white" });
    });
    cinema.addEventListener('change', () => {
        sendParameter({ mode: "cinema" });
    });
    mood.addEventListener('change', () => {
        sendParameter({ mode: "moodlight" });
    });

    colorwheel.addEventListener('click', (event) => {
        const x = event.offsetX - colorwheel.offsetWidth / 2.0;
        const y = - (event.offsetY - colorwheel.offsetHeight / 2.0);
        
        let deg = Math.atan2(x, y) / Math.PI * 180.0;
        if (deg < 0) deg += 360;

        let dist = Math.abs(Math.sqrt(Math.pow(x, 2) + Math.pow(y, 2)))
        if (dist > 144) dist = 144;

        sendParameter({
            saturation: Math.round(dist / 144.0 * 100.0) / 100.0,
            hue: Math.round(deg / 360.0 * 100.0) / 100.0,
        });
    });
}

attachEventHandlers();