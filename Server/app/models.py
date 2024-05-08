from datetime import datetime

from werkzeug.security import generate_password_hash,  check_password_hash

from flask import url_for

from flask_login import UserMixin

from app import db, login_manager


# Функция для загрузки объекта текущего пользователя в глобаьную переменную current_user
@login_manager.user_loader
def load_user(user_id):
    return User.query.get(int(user_id))


class User(db.Model, UserMixin):
    __tablename__ = 'users'
    id = db.Column(db.Integer(), primary_key=True)
    username = db.Column(db.String(64), index=True, unique=True, nullable=False)
    email = db.Column(db.String(120), index=True, unique=True)
    password_hash = db.Column(db.String(128), nullable=False)
    created_on = db.Column(db.DateTime(), default=datetime.utcnow)
    updated_on = db.Column(db.DateTime(), default=datetime.utcnow, onupdate=datetime.utcnow)
    is_admin = db.Column(db.Boolean(), default=False)
    
    def set_password(self, password):
        self.password_hash = generate_password_hash(password)

    def check_password(self,  password):
        return check_password_hash(self.password_hash, password)

    def __repr__(self):
        return f"<{self.id}:{self.username}>"


class Device(db.Model):
    __tablename__ = 'devices'
    id = db.Column(db.Integer(), primary_key=True)
    name = db.Column(db.String(64), index=True, unique=True, nullable=False)
    ip = db.Column(db.String(15), index=True, unique=True, nullable=False)
    note = db.Column(db.Text(), nullable=True)
    position_index = db.Column(db.Integer(), default=10)

        
    def __repr__(self) -> str:
        return f"<{self.id}:{self.name}:{self.ip}>"