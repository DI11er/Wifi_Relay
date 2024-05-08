import logging

from flask import abort, request, url_for, redirect, flash, render_template, make_response

from flask_socketio import join_room

from flask_login import login_required, current_user, login_user, logout_user

from werkzeug.urls import url_parse

from app import app, socketio, db

from app.models import User, Device

from app.forms import AddDeviceForm, EditDeviceForm, LoginForm, RemoveDeviceForm

from app.utils import notify_telegram

_log = logging.getLogger(__name__)



# Представление для загрузки профиля пользователя
@app.route('/profile/')
@login_required
def profile():
    context = {
        'title': 'Страница пользователя',
        'menu': (
            {'url': url_for('add_device'), 'title': 'Добавить устройство'},
        ),
    }
    
    resp = make_response(render_template('profile.html', **context))
    
    return resp

# Представление для отображения формы авторизации
@app.route('/login/', methods=['post',  'get'])
def login():
    if current_user.is_authenticated:
        return redirect(url_for('index'))

    form = LoginForm(request.form)
    if form.validate_on_submit():
        user = User.query.filter_by(username=form.username.data).first()
        if user and user.check_password(form.password.data):
            login_user(user, remember=form.remember_me.data)
            # Обратное перенаправление пользователя на первоначальную страницу
            next_page = request.args.get('next')
            if not next_page or url_parse(next_page).netloc != '':
                next_page = url_for('index')
            return redirect(next_page)
        
        flash("Неправильное имя пользователя или пароль", 'error')
        return redirect(url_for('login'))
    
    context = {
        'title': 'Окно авторизации',
        'form': form
    }

    resp = make_response(render_template('login.html', **context))

    return resp

# Функция для завершения теущего сеанса поьзователя и очистки куков
@app.route('/logout/')
@login_required
def logout():
    logout_user()
    flash("Текущий сеанс завершён", 'success')
    return redirect(url_for('login'))

# Представление для отображения главной таблицы с реле
@app.route('/')
@app.route('/index')
@login_required
def index():
    context = {
        'title': 'Меню управения',
        'devices': Device.query.order_by(Device.position_index),
        'menu': (
            {'url': 'add_device', 'title': 'Добавить устройство'},
        )
    }
    
    resp = make_response(render_template('index.html', **context))
    
    return resp

# Представление для отображения формы добавления новых устройств
@app.route('/add_device/', methods=['post', 'get'])
@login_required
def add_device():
    form = AddDeviceForm()
    
    if form.validate_on_submit():
        d = Device(
            name=form.name.data, 
            ip=form.ip.data,
            note=form.note.data,
            position_index = form.position_index.data
        )
        
        try:
            db.session.add(d)
            db.session.commit()
            flash("Устройство зарегистрировано", "success")
        except Exception as _ex:
            _log.error(_ex)
            db.session.rollback()
            flash("Ошибка при регистрации устройства", "error")
  
    context = {
        'title': 'Регистрация устройства',
        'menu': (
        ),
        'form': form
    }

    resp = make_response(render_template('add_device.html', **context))
    
    return resp

# Представление для отображения формы редактирования устройств
@app.route('/edit_device/<int:id_device>/', methods=['post', 'get'])
@login_required
def edit_device(id_device):
    d: Device = db.session.query(Device).get(id_device)
    form = EditDeviceForm(obj=d)
    
    if form.validate_on_submit():
        d.name=form.name.data 
        d.ip=form.ip.data
        d.note=form.note.data
        d.position_index = form.position_index.data

        try:
            db.session.commit()
            flash("Данные изменены", "success")
        except Exception as _ex:
            _log.error(_ex)
            flash("Ошибка при изменении данных", "error")
            db.session.rollback()
            
    context = {
        'title': 'Настройка устройства',
        'menu': (
            {'url': url_for('add_device'), 'title': 'Регистрация устройства'},
        ),
        'form': form
    }

    resp = make_response(render_template('edit_device.html', **context))
    
    return resp

# Представление для отображения формы удаления устройств
@app.route('/remove_device/<int:id_device>/', methods=['post', 'get'])
@login_required
def remove_device(id_device):
    d: Device = db.session.query(Device).get(id_device)
    form = RemoveDeviceForm(obj=d)
    
    if form.validate_on_submit():
        try:
            db.session.delete(d)
            db.session.commit()
            flash("Устройство удалено", "success")
        except Exception as _ex:
            _log.error(_ex)
            flash("Ошибка при удалении устройства", "error")
            db.session.rollback()
        
        return redirect(url_for('index'))
    
    context = {
        'title': 'Удаление устройства',
        'menu': (
            {'url': url_for('add_device'), 'title': 'Регистрация устройства'},
        ),
        'form': form
    }

    resp = make_response(render_template('remove_device.html', **context))
    
    return resp

""" WebSocket Handlers """

# Обработчик события connect
@socketio.on('connect')
def handle_connect():
    _log.debug(f'Client connected {request.remote_addr}')

# Обработчик события disconnect
@socketio.on('disconnect')
def handle_disconnect():
    _log.debug(f'Client disconnected {request.remote_addr}')
    socketio.emit('toogle_status', {'ip': request.remote_addr}, room='frontend')

# Обработчик события control_relay
@socketio.on('control_relay')
def control_relay_event(response: dict):
    _log.debug(f'control_relay {response}')
    socketio.emit('control_relay', response, room='relay')

# Обработчик события control_voltage    
@socketio.on('control_voltage')
def control_voltage_event(response: dict):
    _log.debug(f'control_voltage {response}')
    socketio.emit('control_voltage', response, room='relay')
    
@socketio.on('control_sensor')
def control_sensor_event(response: dict):
    _log.debug(f'control_sensor {response}')
    socketio.emit('control_sensor', response, room='relay')

# Обработчик события relay_parameters        
@socketio.on('relay_parameters')
def relay_parameters_event(response: dict):
    _log.debug(f'relay_parameters {response}')
    socketio.emit('update_relay_parameters', response, room='frontend')
    
# Обработчик события join_room
@socketio.on('join_room')
def on_join(response):
    _log.debug(f'Client join_room {request.remote_addr} -> {response}')
    username = response['username']
    room = response['room']
    join_room(room)
    socketio.emit('room_message', username + ' has joined the room.', room=room)
    
# Обработчик события alarm_signal
@socketio.on('alarm_signal')
def on_join(response):
    _log.debug(f'alarm_signal {response}')
    notify_telegram(f"Реле {response['ip']}\nСрабатывание датчика -> {response['chanel']['description']}")
    
# Обработчик события voltage_low_signal
@socketio.on('voltage_low_signal')
def on_join(response):
    _log.debug(f'voltage_low_signal {response}')
    notify_telegram(f"Реле {response['ip']}\nНизкий заряд на входе {response['chanel']['name']} -> {round(response['chanel']['voltage'], 3)}")