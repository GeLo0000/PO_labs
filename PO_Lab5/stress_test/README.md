# 1. Деактивувати
deactivate

# 2. Видалити стару папку venv
Remove-Item -Recurse -Force .\venv

# 3. Створити нову віртуалку (перебуваючи в папці ai_e2e_testing):
python -m venv venv

# 4. Активувати:
.\venv\Scripts\activate

# 5. Встановити залежності:
pip install -r requirements.txt

# 5. Запустити тест:

locust -f test.py