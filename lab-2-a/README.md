Parte A: tuvimos que deshabilitar el watchdog con el esp_task_wdt_deinit();

E (65249) task_wdt: Task watchdog got triggered. The following tasks/users did not reset the watchdog in time:
E (65249) task_wdt:  - IDLE (CPU 0)
E (65249) task_wdt: Tasks currently running:
E (65249) task_wdt: CPU 0: main
E (65249) task_wdt: Print CPU 0 (current core) backtrace