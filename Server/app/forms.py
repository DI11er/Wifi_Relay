from flask_wtf import FlaskForm

from app.models import Device

from wtforms import StringField, SubmitField, TextAreaField, BooleanField, IntegerField, PasswordField, ValidationError
from wtforms.validators import DataRequired, IPAddress


class BaseForm(FlaskForm):
    pass


class LoginForm(BaseForm):
    username = StringField('Имя пользователя: ', validators=[DataRequired()])
    password = PasswordField('Пароль пользователя: ', validators=[DataRequired()])
    remember_me = BooleanField('Запомнить меня: ', default=False)
    submit = SubmitField('Войти')


class BaseFormDevice(BaseForm):
    name = StringField('Название устройства: ', validators=[DataRequired()])
    ip = StringField('Ip-адрес устройства: ', validators=[DataRequired(), IPAddress()])
    note = TextAreaField('Примечание: ')
    position_index = IntegerField('Порядок: ', default=10)
    


class AddDeviceForm(BaseFormDevice):
    submit = SubmitField('Зарегистрировать устройство')
    
    # Валидатор для поля name, проверяет поле name на уникальность
    def validate_name(self, name):
        if Device.query.filter_by(name=name.data).first() is not None:
            raise ValidationError('Пожалуйста используйте другое имя.')
    
    # Валидатор для поля ip, проверяет поле ip на уникальность
    def validate_ip(self, ip):
        if Device.query.filter_by(ip=ip.data).first() is not None:
            raise ValidationError('Пожалуйста используйте другой ip-адрес.')
    
    
class EditDeviceForm(BaseFormDevice):
    submit = SubmitField('Обновить данные устройства')
 
    
class RemoveDeviceForm(BaseFormDevice):
    submit = SubmitField('Удалить устройство')