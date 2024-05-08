# -*- coding: utf-8 -*-

import logging
from logging.handlers import RotatingFileHandler, SMTPHandler
from pathlib import Path

from flask import Flask
from flask_sqlalchemy import SQLAlchemy
from flask_migrate import Migrate
from flask_socketio import SocketIO
from flask_login import LoginManager
from flask_moment import Moment

from config import env, resources_dir

_log = logging.getLogger(__name__)

app = Flask(__name__)
app.config.from_object(env.str('FLASK_CONFIG', 'config.DevelopementConfig'))

db = SQLAlchemy(app)

migrate = Migrate(app, db)

socketio = SocketIO(app, cors_allowed_origins="*", logger=True, engineio_logger=True)

login_manager = LoginManager(app)
login_manager.login_message = 'Пожалуйста, войдите, чтобы открыть эту страницу.'
login_manager.login_view = 'login'

moment = Moment(app)

if not app.debug:
    if app.config['MAIL_SERVER']:
        auth = None
        if app.config['MAIL_USERNAME'] or app.config['MAIL_PASSWORD']:
            auth = (app.config['MAIL_USERNAME'], app.config['MAIL_PASSWORD'])
        secure = None
        if app.config['MAIL_USE_TLS']:
            secure = ()
        mail_handler = SMTPHandler(
            mailhost=(app.config['MAIL_SERVER'], app.config['MAIL_PORT']),
            fromaddr='no-reply@' + app.config['MAIL_SERVER'],
            toaddrs=app.config['ADMINS'], subject='Server_relay Failure',
            credentials=auth, secure=secure)
        mail_handler.setLevel(logging.ERROR)
        app.logger.addHandler(mail_handler)
        
        
    if not Path.exists(resources_dir / 'logs'):
        Path.mkdir(resources_dir / 'logs')
        
    file_handler = RotatingFileHandler(resources_dir / 'logs' / 'server_relay.log', maxBytes=10240,
                                       backupCount=10)
    file_handler.setFormatter(logging.Formatter(
        '%(asctime)s %(levelname)s: %(message)s [in %(pathname)s:%(lineno)d]'))
    file_handler.setLevel(logging.INFO)
    app.logger.addHandler(file_handler)

    app.logger.setLevel(logging.INFO)
    app.logger.info('Server_relay startup')


from app import routes, models, admin, cli, error