@echo off
venv/Script/activate
flask db init
flask db migrate
flask db upgrade
python server_relay.py flask run