import logging

from pathlib import Path

from environs import Env

env = Env()
env.read_env()

app_dir = Path.cwd() / env.str('WORK_DIR', default='.')

resources_dir = app_dir / 'resources'


class ConfigAppRun:
    HOST = env.str('HOST', '0.0.0.0')
    PORT = env.int('PORT', 4145)
    USE_RELOADER = env.bool('USE_RELOADER', default=True)
    

class ConfigNotifyTelegram:
    BOT_TOKEN = env.str('BOT_TOKEN', '')
    CHAT_ID = env.str('CHAT_ID', '')


class BaseConfig:
    FLASK_ADMIN_SWATCH = 'cerulean'
    SECRET_KEY = env.str('SECRET_KEY', 'adadad198i99A8DS787s87S890')
    SQLALCHEMY_TRACK_MODIFICATIONS = False

    ##### настройка Flask-Mail ##### 
    MAIL_SERVER = env.str('MAIL_SERVER', 'localhost')
    MAIL_PORT = env.int('MAIL_PORT', 25)
    MAIL_USE_TLS = env.str('MAIL_USE_TLS', None)
    MAIL_USERNAME = env.str('MAIL_USERNAME')
    MAIL_PASSWORD = env.str('MAIL_PASSWORD')
    ADMINS = ['your-email@example.com']


class DevelopementConfig(BaseConfig):
    DEBUG = True
    SQLALCHEMY_DATABASE_URI = env.str('DEVELOPMENT_DATABASE_URI', default=f"sqlite:///{resources_dir / 'db' / 'app.db'}")


class TestingConfig(BaseConfig):
    DEBUG = True
    SQLALCHEMY_DATABASE_URI = env.str('TESTING_DATABASE_URI', default=f"sqlite:///{resources_dir / 'db' / 'app.db'}")
	
    # 'mysql+pymysql://root:pass@localhost/flask_app_db'


class ProductionConfig(BaseConfig):
    DEBUG = False
    SQLALCHEMY_DATABASE_URI = env.str('PRODUCTION_DATABASE_URI', default=f"sqlite:///{resources_dir / 'db' / 'app.db'}")
