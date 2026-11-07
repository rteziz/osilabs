# Пользовательские потоки

Нужен запущенный Docker Desktop (на Linux — Docker с Compose).

Из корня проекта перейди в папку лабы и запусти:

```sh
cd mythread_usr_aga
docker compose run --build --rm uthread
```

Команда собирает библиотеку, выполняет тесты и запускает пример.
После изменения кода повтори команду `docker compose run --build --rm uthread`.
Первая сборка требует интернета для загрузки образа и компилятора.

Отдельно запустить тесты после сборки:

```sh
docker compose run --rm uthread ctest --test-dir build --output-on-failure
```

Из корня общего проекта та же команда запуска работает без `cd`.

Подробное объяснение кода: [EXPLANATION.md](EXPLANATION.md).
