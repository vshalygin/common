#include "app.h"
#include "../utils/utils.h"

#include <rpc-lib/authenticator/simple-authenticator/simple-authenticator.h>
#include <rpc-lib/pipe/memory-pipe/mem-pipe-env.h>
#ifdef _WIN32
#include <rpc-lib/pipe/win-pipe/win-pipe-client-env.h>
#include <rpc-lib/pipe/win-pipe/win-pipe-server-env.h>
#endif
#include <rpc-lib/pipe/tcp-pipe/tcp-pipe-client-env.h>
#include <rpc-lib/pipe/tcp-pipe/tcp-pipe-server-env.h>

#include <boost/algorithm/string.hpp>

#include <charconv>
#include <sstream>
#include <optional>
#include <utility>
#include <tuple>

namespace vshalygin::example {
    namespace {
        const std::string s_help_command = "help";
        const std::string s_exit_command = "exit";
        const std::string s_info_command = "info";
        const std::string s_add_client_command = "add client";
        const std::string s_remove_client_command_begin = "remove client";
        const std::string s_send_to_server_command_begin = "send to server";
        const std::string s_send_to_client_command_begin = "send to client";
        const std::string s_send_to_all_clients_command_begin = "send to all clients";

        enum class command_type
        {
            unknown,
            help,
            exit,
            info,
            add_client,
            remove_client,
            send_to_server,
            send_to_client,
            send_to_all_clients
        };

        enum class transport_type
        {
            memory_pipe = 0,
#ifdef _WIN32
            win_pipe = 1,
#endif
            tcp = 2,

            unknown = 3
        };

        std::string create_choose_transport_prompt()
        {
            std::stringstream ss;
            ss << "Choose transport: \n"
               << "0 - memory pipe\n";
#ifdef _WIN32
            ss << "1 - win pipe\n";
#endif
            ss << "2 - tcp\n";


            return ss.str();
        }

        transport_type parse_chosen_transport(const std::string &line)
        {
            if(line == "0") {
                return transport_type::memory_pipe;
            }
#ifdef _WIN32
             else if (line == "1") {
                return transport_type::win_pipe;
            }
#else
             else if (line == "2") {
                return transport_type::tcp;
#endif
            } else {
                return transport_type::unknown;
            }
        }

        command_type parse_command(const std::string &line)
        {
            command_type ans = command_type::unknown;
            if(line == s_help_command) {
                ans = command_type::help;
            } else if (line == s_exit_command) {
                ans = command_type::exit;
            } else if (line == s_info_command) {
                ans = command_type::info;
            } else if (line == s_add_client_command) {
                ans = command_type::add_client;
            } else if (boost::algorithm::starts_with(line, s_remove_client_command_begin)) {
                ans = command_type::remove_client;
            } else if (boost::algorithm::starts_with(line, s_send_to_server_command_begin)) {
                ans = command_type::send_to_server;
            } else if (boost::algorithm::starts_with(line, s_send_to_client_command_begin)) {
                ans = command_type::send_to_client;
            } else if (boost::algorithm::starts_with(line, s_send_to_all_clients_command_begin)) {
                ans = command_type::send_to_all_clients;
            }

            return ans;
        }

        size_t skip_whitespace(const std::string &line, size_t pos) noexcept
        {
            while(pos < line.size() &&
                  (line[pos] == ' ' || line[pos] == '\t' || line[pos] == '\r' || line[pos] == '\n'))
            {
                ++pos;
            }

            return pos;
        }

        std::optional<uint64_t> extract_number_after_command(const std::string &line,
                                                             const std::string &command,
                                                             bool require_end)
        {
            if(line.size() <= command.size() ||
               skip_whitespace(line, command.size()) == command.size())
            {
                return std::nullopt;
            }

            const auto number_begin = skip_whitespace(line, command.size());
            auto number_end = number_begin;
            while(number_end < line.size() && line[number_end] >= '0' && line[number_end] <= '9') {
                ++number_end;
            }

            if(number_begin == number_end ||
               (number_end < line.size() && skip_whitespace(line, number_end) == number_end))
            {
                return std::nullopt;
            }

            uint64_t number = 0;
            const auto parse_result = std::from_chars(line.data() + number_begin,
                                                      line.data() + number_end,
                                                      number);
            if(parse_result.ec != std::errc{} || parse_result.ptr != line.data() + number_end) {
                return std::nullopt;
            }

            if(require_end && skip_whitespace(line, number_end) != line.size()) {
                return std::nullopt;
            }

            return number;
        }

        std::optional<std::string> extract_message_after_number(const std::string &line,
                                                                const std::string &command)
        {
            if(line.size() <= command.size() ||
               skip_whitespace(line, command.size()) == command.size())
            {
                return std::nullopt;
            }

            const auto number_begin = skip_whitespace(line, command.size());
            auto number_end = number_begin;
            while(number_end < line.size() && line[number_end] >= '0' && line[number_end] <= '9') {
                ++number_end;
            }

            if(number_begin == number_end ||
               number_end == line.size() ||
               skip_whitespace(line, number_end) == number_end)
            {
                return std::nullopt;
            }

            const auto message_begin = skip_whitespace(line, number_end);
            if(message_begin == line.size()) {
                return std::nullopt;
            }

            return line.substr(message_begin);
        }

        std::optional<std::string> extract_message_after_command(const std::string &line,
                                                                 const std::string &command)
        {
            if(line.size() <= command.size() ||
               skip_whitespace(line, command.size()) == command.size())
            {
                return std::nullopt;
            }

            const auto message_begin = skip_whitespace(line, command.size());
            if(message_begin == line.size()) {
                return std::nullopt;
            }

            return line.substr(message_begin);
        }

        std::optional<uint64_t> extract_client_id_to_remove(const std::string &line)
        {
            assert(line.size() >= s_remove_client_command_begin.size());

            return extract_number_after_command(line, s_remove_client_command_begin, true);
        }

        std::optional<uint64_t> extract_client_id_to_send_from(const std::string &line)
        {
            assert(line.size() >= s_send_to_server_command_begin.size());

            return extract_number_after_command(line, s_send_to_server_command_begin, false);
        }

        std::optional<std::string> extract_message_to_send_from_client(const std::string &line)
        {
            assert(line.size() >= s_send_to_server_command_begin.size());

            return extract_message_after_number(line, s_send_to_server_command_begin);
        }

        void print_help()
        {
            std::stringstream ss;
            ss << "Available commands:\n"
               << "  " << s_help_command << " - show this help\n"
               << "  " << s_info_command << " - show clients and server connections\n"
               << "  " << s_add_client_command << " - create and connect a new client\n"
               << "  " << s_remove_client_command_begin << " <client-id> - remove a client\n"
               << "  " << s_send_to_server_command_begin
               << " <client-id> <message> - send a message from a client to the server\n"
               << "  " << s_send_to_client_command_begin
               << " <connection-id> <message> - send a message from the server to one client\n"
               << "  " << s_send_to_all_clients_command_begin
               << " <message> - send a message from the server to all clients\n"
               << "  " << s_exit_command << " - finish the application";

            write_to_console(ss.str() + "\n");
        }
    }

    app::app()
        : m_thread_pool(std::make_shared<cl::thread_pool>(2))
        , m_authenticator(std::make_shared<rpc::simple_authenticator>())
    {}

    int app::run() noexcept
    {
        try {
            transport_type transport = transport_type::unknown;
            while(transport == transport_type::unknown) {
                write_to_console(create_choose_transport_prompt() + "\n");

                write_to_console(">");
                auto line = read_line_from_console();
                if(!line) {
                    throw std::runtime_error("cannot read from console");
                }
                boost::algorithm::trim(*line);
                transport = parse_chosen_transport(*line);
            }

            if(transport == transport_type::memory_pipe) {
                auto env = std::make_shared<rpc::mem_pipe_env>(m_thread_pool.get());
                m_client_pipe_env = env;
                m_server_pipe_env = env;
#ifdef _WIN32
            } else if (transport == transport_type::win_pipe) {
                m_client_pipe_env = std::make_shared<rpc::win_pipe_client_env>(m_thread_pool.get(), L"47sdfrtgvczc849dsbdevdedb");
                m_server_pipe_env = std::make_shared<rpc::win_pipe_server_env>(m_thread_pool.get(), L"47sdfrtgvczc849dsbdevdedb");
#endif
            } else if (transport == transport_type::tcp) {
                m_client_pipe_env = std::make_shared<rpc::tcp_pipe_client_env>(m_thread_pool.get(), "127.0.0.1", 31078);
                m_server_pipe_env = std::make_shared<rpc::tcp_pipe_server_env>(m_thread_pool.get(), "127.0.0.1", 31078);
            } else {
                throw std::runtime_error("unknown transport type");
            }

            m_server = std::make_unique<server>(m_thread_pool.get(), m_authenticator, m_server_pipe_env);

            command_type command = command_type::unknown;
            do {
                write_to_console(">");
                auto line = read_line_from_console();
                if(!line) {
                    throw std::runtime_error("cannot read from console");
                }
                boost::algorithm::trim(*line);

                command = parse_command(*line);
                switch(command) {
                    case command_type::unknown:
                        write_to_console("Unknown command. Print 'help' for information\n");
                        break;
                    case command_type::help:
                        print_help();
                        break;
                    case command_type::info:
                        print_info();
                        break;
                    case command_type::exit:
                        write_to_console("Exit command entered. Finish application...\n");
                        break;
                    case command_type::add_client:
                    {
                        auto id = m_next_client_id++;
                        try {
                            m_clients.emplace(std::piecewise_construct,
                                              std::forward_as_tuple(id),
                                              std::forward_as_tuple(m_thread_pool.get(), m_authenticator, m_client_pipe_env, id));
                            write_to_console("Client with id " + std::to_string(id) + " created\n");
                        } catch (const std::exception &e) {
                            write_to_console("Failed to create client: " + std::string(e.what()) + "\n");
                        }
                        break;
                    }
                    case command_type::remove_client:
                    {
                        auto id = extract_client_id_to_remove(*line);
                        if(!id) {
                            write_to_console("Unable to extract client id to remove. Print 'help' for information\n");
                        } else if (!m_clients.count(*id)) {
                            write_to_console("No client with specified id\n");
                        } else {
                            m_clients.erase(*id);
                            write_to_console("Client with id " + std::to_string(*id) + " removed\n");
                        }
                        break;
                    }
                    case command_type::send_to_server:
                    {
                        auto id = extract_client_id_to_send_from(*line);
                        if(!id) {
                            write_to_console("Unable to extract client id to send from. Print 'help' for information\n");
                        } else if (!m_clients.count(*id)) {
                            write_to_console("No client with specified id\n");
                        } else {
                            auto message = extract_message_to_send_from_client(*line);
                            if(!message) {
                                write_to_console("Unable to extract message to send from client. Print 'help' for information\n");
                            } else {
                                m_clients.at(*id).send(*message);
                            }
                        }
                        break;
                    }
                    case command_type::send_to_client:
                    {
                        auto connection_id = extract_number_after_command(*line,
                                                                          s_send_to_client_command_begin,
                                                                          false);
                        if(!connection_id) {
                            write_to_console("Unable to extract connection id to send to. Print 'help' for information\n");
                        } else {
                            auto message = extract_message_after_number(*line, s_send_to_client_command_begin);
                            if(!message) {
                                write_to_console("Unable to extract message to send to client. Print 'help' for information\n");
                            } else {
                                m_server->send(*connection_id, *message);
                            }
                        }
                        break;
                    }
                    case command_type::send_to_all_clients:
                    {
                        auto message = extract_message_after_command(*line,
                                                                     s_send_to_all_clients_command_begin);
                        if(!message) {
                            write_to_console("Unable to extract message to send to all clients. Print 'help' for information\n");
                        } else {
                            m_server->send_all(*message);
                        }
                        break;
                    }
                    default:
                        throw std::runtime_error("unknown command");
                }
            } while(command != command_type::exit);

            return 0;
        } catch (const std::exception &e) {
            write_to_console(std::string("ERROR: ") + e.what() + "\n");
        }

        return 1;
    }

    void app::print_info()
    {
        std::stringstream ss;
        ss << "Clients count: " << m_clients.size() << '\n'
           << "Client ids: ";

        if(m_clients.empty()) {
            ss << "none";
        } else {
            bool first = true;
            for(const auto &entry : m_clients) {
                if(!first) {
                    ss << ", ";
                }
                ss << entry.first;
                first = false;
            }
        }

        ss << '\n' << "Server connections count: " << m_server->get_connections_count();
        write_to_console(ss.str() + "\n");
    }
}
