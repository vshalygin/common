#pragma once
#include <common-lib/utils/buffer/buffer.h>

#include <functional>

namespace vshalygin::rpc {
    /*TODO написать все красиво на англицком
        1) transport создается не запущенным, нужно явно вызывать start
        2) start бросает исключение в случае, если за удалось запусить транспорт
        3) start_callback вызывается сразу после успешного запуска транспорта в любом потоке
        4) stop_callback вызывается в случае, если вызван stop, если транспорт самопроизвольно
        остановился, если транспорт уничтожается
        5) Каждому вызову start_callback должен соответствовать вызов stop_callback
        6) stop_callback должен вызываться гарантированно после завершения start_callback
        !7) Если колбеки stop_callback или start_callback не установлены, то они не вызываются
        8) Если recv_async бросает исключение, то transport должен находится в состоянии stopped.
        Соответствующие колбеки должный вызываться
        9) send_async может бросать исключение даже если состояние не stopped. В случае ошибки отправки
        сообщения должен вызываться колбек error_handler.
        10) После завершения start и stop транспорт гарантированно находится в новом состоянии
        11) stop не должен бросать исключения, хотя он и не объявлен явно как noexcept
        !12) В случае, если stop_callback и start_callback были установлены после наступления
        соответствующих событий, то они не вызываются
        !13) stop должен завершать все recv_async и send_async. Вызов error_handler для send_async не
        требуется
        14) Перед вызовом stop_callback должны быть очищены все pending read и write
        15) send_async может и должен бросать исключения, если нет возможности отправить
    */

    //TODO write requesties
    class itransport
    {
    public:
        virtual ~itransport() = default;

        virtual void send_async(cl::buffer &&message,
                                std::function<void()> &&error_handler) const = 0;
        virtual void recv_async(std::function<void(cl::buffer &&)> &&handler) const = 0;

        virtual void start(std::function<void()> &&start_callback,
                           std::function<void()> &&stop_callback) = 0;
        virtual void stop() = 0;
        virtual bool is_stopped() const = 0;
    };
}
