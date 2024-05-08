const socket = io.connect(window.location.protocol + '//' + window.location.hostname + ':' + window.location.port);

socket.emit('join_room', {
    'username': 'Web-GUI',
    'room': 'frontend'
})

socket.on('connect_error', (error) => {
    console.error('Connection Error:', error);
})

socket.on('toogle_status', function(json_data) {
    let ip = json_data['ip'];

    let main_table = document.getElementById('device_table');

    Array.prototype.slice.call(main_table.querySelectorAll('tr'), 1, ).forEach(function(rows) {
        if (rows.querySelector('.ip').innerText == ip) {
            rows.querySelectorAll('.status').forEach(function(block) {
                block.innerHTML = '<div class="circle" style="background-color: grey;"> </div>'
            })

            rows.querySelector('.inner_voltage_levels').innerText = '';
            rows.querySelector('.inner_relay_chanels').innerText = '';
            rows.querySelector('.inner_sensor_chanels').innerText = '';
        }
    })
})


const static_dir_img = "/static/img/";

socket.on('update_relay_parameters', function(json_data) {
    let main_table = document.getElementById('device_table');

    let data = json_data['data'];

    let ip_relay = data['ip_relay'];

    Array.prototype.slice.call(main_table.querySelectorAll('tr'), 1, ).forEach(function(rows) {
        if (rows.querySelector('.ip').innerText == ip_relay) {
            let voltage_chanel = rows.querySelector('.voltage_levels > div.inner_voltage_levels');
            let relay_chanel = rows.querySelector('.relay_chanels > div.inner_relay_chanels');
            let sensor_chanel = rows.querySelector('.sensor_chanels > div.inner_sensor_chanels');

            rows.querySelector('.status').innerHTML = '<div class="circle" style="background-color: green;"> </div>';

            let temp = "";

            data['voltage_info'].forEach(function(u){
                temp += `<div class="chanel">
                    <div class="name_chanel">` + 
                        u['name'] + ':' +
                    `</div>
                    <div class="voltage">` +
                        u['voltage'].toFixed(3)  +
                    `</div>
                    <div class="btn">
                        <button type="button">
                            <img src="` + static_dir_img + `indicator.ico" onclick="check_click('` + ip_relay + `', 'voltage', '` + u['name'] + `', 'measure')">
                        </button>
                    </div>
                </div>`
            })

            voltage_chanel.innerHTML = temp;
            temp = "";

            data['relay_info'].forEach(function(r){
                let color;
                if (r['state']) {
                    color = 'green';
                } else { color = 'red'; }

                temp += `<div class="chanel">
                    <div class="name_chanel">` +
                        r['name'] + ':' +
                    `</div>
                    <div class="btn">
                        <button type="button" >
                            <img src="` + static_dir_img + `toggle.ico" onclick="check_click('` + ip_relay + `', 'reley', '` + r['name'] + `', 'toggle')">
                        </button>
                        <button type="button" >
                            <img src="` + static_dir_img + `synchronize.ico" onclick="check_click('` + ip_relay + `', 'reley', '` + r['name'] + `', 'restart')">
                        </button>
                    </div>
                    <div class="state">
                        <div class="circle" style="background-color:` + color + `;"> </div>
                    </div>
                </div>`
            })

            relay_chanel.innerHTML = temp;
            temp = "";

            data['sensor_info'].forEach(function(s){
                let color;
                if (s['state_sensor']) {
                    color = 'green';
                } else { color = 'red'; }

                let color1 = 'none';
                if (s['alarm_signal'] == 1) {
                    color1 = 'red';
                } else if (s['alarm_signal'] == 0) { color1 = 'green'; }

                temp += `<div class="chanel">
                    <div class="name_chanel">` +
                        s['name'] + ':' +
                    `</div>
                    <div class="btn">
                        <button type="button" >
                            <img src="` + static_dir_img + `toggle.ico" onclick="check_click('` + ip_relay + `', 'sensor', '` + s['name'] + `', 'toggle')">
                        </button>
                    </div>
                    <div class="state">
                        <div class="circle" style="background-color:` + color + `;"> </div>
                    </div>
                    <div class="state alarm">
                        <div class="circle" style="background-color:` + color1 + `;"> </div>
                    </div>
                </div>`
            })

            sensor_chanel.innerHTML = temp;
            temp = "";
        }
    })
})


function send_data_socketio(event, ip, number_chanel, action) {
    socket.emit(event, {
        'ip': ip,
        'data': {
            'channel': {
                'name': number_chanel,
                'action': action
            }
        }
    });
}


function check_click(ip, type_device, number_chanel, action) {
    if (type_device == 'reley') {
        send_data_socketio('control_relay', ip, number_chanel, action);
    } else if (type_device == 'voltage') {
        send_data_socketio('control_voltage', ip, number_chanel, action);
    } else if (type_device == 'sensor') {
        send_data_socketio('control_sensor', ip, number_chanel, action);
    } else {
        socket.emit('error_handler', {
            'ip': ip,
            'data': {
                'message': 'Такое событие не обрабатывается'
            }
        })
    }
}

document.cookie = "foo=bar;";

if (!document.cookie) {
	alert("This website requires cookies to function properly");
}