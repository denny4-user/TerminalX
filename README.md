# TerminalX

Прошивка-мультитул для **M5Stack StickS3** (ESP32-S3, 8 МБ). Базируется на проверенном
форке [Bruce](https://github.com/pr3y/Bruce) (**Bruce-A-C-Edition**), собранном только под
этот борд. Первый рабочий модуль — **IR** (клон пультов + TV-B-Gone).

Веб-флешер (прошивка из браузера, Chrome/Edge): **https://denny4-user.github.io/TerminalX/**

> На устройстве интерфейс пока брендирован как Bruce — переименование в TerminalX
> и обрезка лишних функций идут по мере тестирования (по очереди, начиная с IR).

## Сборка

Нужен [PlatformIO](https://platformio.org/).

```bash
pio run -e m5stack-sticks3          # собрать (default env)
pio run -e m5stack-sticks3 -t upload
```

После сборки `build.py` кладёт готовый склеенный образ `Bruce-m5stack-sticks3.bin`
(bootloader@0x0 + partitions@0x8000 + app@0x10000). Для веб-флешера он копируется в
`docs/firmware/TerminalX.bin`.

### Заметка про сборку (net80211)

`patch.py` ослабляет символ `ieee80211_raw_frame_sanity_check` в `libnet80211.a`
(нужно для инъекции raw-фреймов WiFi). Если в скачанном `framework-arduinoespressif32-libs`
для esp32s3 патч не отработал (либа осталась как `libnet80211.a.old`), верни её и собери с
`-Wl,--allow-multiple-definition` (уже прописан в `platformio.ini`):

```bash
LIB=~/.platformio/packages/framework-arduinoespressif32-libs/esp32s3/lib
cp "$LIB/libnet80211.a.old" "$LIB/libnet80211.a"
```

## Управление (M5StickS3)

| Кнопка | Функция |
| --- | --- |
| Передняя «M5» (G11) | выбрать / OK |
| Боковая (G12) | листать / назад |

## Лицензия и благодарности

Основано на [Bruce](https://github.com/pr3y/Bruce) (и форке Bruce-A-C-Edition).
Распространяется под **AGPL-3.0** — см. [LICENSE](LICENSE). Огромная благодарность
команде Bruce и bmorcelli за проделанную работу.
