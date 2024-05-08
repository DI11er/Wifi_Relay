import requests

from config import ConfigNotifyTelegram


def notify_telegram(message: str):
    if not ConfigNotifyTelegram.BOT_TOKEN:
        raise Exception('Not set Bot tocken')
    
    if not ConfigNotifyTelegram.CHAT_ID:
        raise Exception('Not set chat_id')
    
    telegram_api = f'https://api.telegram.org/bot{ConfigNotifyTelegram.BOT_TOKEN}/sendMessage'
    
    request = requests.post(telegram_api, data={
        'chat_id': ConfigNotifyTelegram.CHAT_ID,
        'text': message,
    })
    
    if request.status_code != 200:
        raise Exception('post_text error')