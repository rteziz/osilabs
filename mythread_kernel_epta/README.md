# Ядерные потоки

Нужен запущенный Docker Desktop (на Linux — Docker с Compose).

Из корня проекта перейди в папку лабы и запусти:

```sh
cd mythread_kernel_epta
docker compose run --build --rm mythread
```

Команда собирает библиотеку, выполняет тесты и запускает пример.
После изменения кода повтори команду `docker compose run --build --rm mythread`.
Первая сборка требует интернета для загрузки образа и компилятора.

Отдельно запустить тесты после сборки:

```sh
docker compose run --rm mythread ctest --test-dir build --output-on-failure
```

Из корня общего проекта та же команда запуска работает без `cd`.
