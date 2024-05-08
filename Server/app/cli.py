import logging

import click

from getpass import getpass

from app import app, db

from app.models import User


_log = logging.getLogger(__name__)



@app.cli.command("create-superuser")
def create_superuser():
    """ Create superuser. """
    name = input('Введит имя пользователя: ')
    if name:
        if not User.query.filter_by(username=name).first():
            email = input('Введите адрес электронной почты: ')
            password = getpass('Введите пароль: ')
            if password == getpass('Введите повторно пароль: '):
                sup = User(username=name, email=email, is_admin=True)
                sup.set_password(password)
                try:
                    db.session.add(sup)
                    db.session.commit()
                    click.echo(f'Пользователь {name} с email {email} создан')
                except Exception as _ex:
                    _log.error(_ex, exc_info=_ex.args)
                    db.session.rollback()
            else:
                click.echo('Пароли не совпали')
        else:
            click.echo('Выберит другое имя пользователя')
    else:
        click.echo('Имя пользователя не может быть пустым')
        