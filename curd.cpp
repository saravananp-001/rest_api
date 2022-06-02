#include <iostream>
#include <vector>
#include <map>
#include <served/served.hpp>
#include <nlohmann/json.hpp>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>

using json = nlohmann::json;

const std::string server = "localhost";
const std::string username = "root";
const std::string password = "S@ravan1n";

sql::Driver *driver;
sql::Connection *con;
sql::Statement *stmt;
sql::PreparedStatement *pstmt;
sql::ResultSet *result;

namespace db
{
    void dbConnection()
    {
        try
        {
            driver = get_driver_instance();
            con = driver->connect(server, username, password);
            std::cout << "db connected\n";
        }
        catch (sql::SQLException e)
        {
            std::cout << "Could not connect to server. Error message: " << e.what() << std::endl;
            system("pause");
            exit(1);
        }
        con->setSchema("userdata");
        stmt = con->createStatement();
    }
}

struct players
{
    std::string name;
    int age;
    int amount;
    std::string team;
};

players PlayerData(std::string p_name, int p_age, int p_amount, std::string team)
{
    players Player;

    Player.name = p_name;
    Player.age = p_age;
    Player.amount = p_amount;
    Player.team = team;
    return Player;
}

void to_json(json &j, const std::string &p)
{
    // j = json{{"name", p.name}, {"age", p.age}, {"amount", p.amount}, {"team", p.team}};
    j = {p};
}

namespace deleteHandle
{
    bool deleteuser(std::string user_name, std::string password, served::response &_res)
    {
        try
        {
            result = stmt->executeQuery("SELECT username,password FROM userlogin");
        }
        catch (sql::SQLException e)
        {
            std::cout << "Error message: " << e.what() << std::endl;
        }

        std::map<std::string, std::string> users;
        while (result->next())
        {
            std::string user = result->getString(1);
            std::string pwd = result->getString(2);
            if (user_name == user)
            {
                if (password == pwd)
                {
                    pstmt = con->prepareStatement("DELETE FROM userlogin WHERE username = ? ");
                    pstmt->setString(1, user_name);
                    try
                    {
                        pstmt->execute();
                        return true;
                    }
                    catch (sql::SQLException e)
                    {
                        std::cout << "Error message: " << e.what() << std::endl;
                        return false;
                    }
                }
                else
                {
                    _res << "wrong password";
                    return false;
                }
            }
        }
        _res << "user doesn't exist";
        return false;
    }
}

namespace Registere
{
    bool userValidation(json credantial, served::response &_res)
    {
        if (!(credantial.contains("userName") && credantial.contains("password")))
        {
            if (!credantial.contains("userName"))
            {
                _res << " wrong input for userName";
            }
            if (!credantial.contains("password"))
            {
                _res << " wrong input for password";
            }
            return false;
        }
        return true;
    }

    bool addUser(std::string _user_name, std::string _password)
    {
        pstmt = con->prepareStatement("INSERT INTO userlogin(username, password) VALUES(?,?)");
        pstmt->setString(1, _user_name);
        pstmt->setString(2, _password);
        try
        {
            pstmt->execute();
            return true;
        }
        catch (sql::SQLException e)
        {
            std::cout << "Error message: " << e.what() << std::endl;
            return false;
        }
        return false;
    }
}

namespace EndpointMethods
{
    constexpr char Register[] = "/register";
    constexpr char UpdateEndpoint[] = "/update";
    constexpr char DeleteEndpoint[] = "/delete";
    constexpr char GetAllEndpoint[] = "/users";
    constexpr char IpAddress[] = "127.0.0.1";
    constexpr char Port[] = "8223";
    constexpr char ThreadCount = 10;

    class Httpserver
    {
    public:
        Httpserver(served::multiplexer mux_) : mux(mux_) {}

        auto RegisterHandler()
        {
            return [&](served::response &res, const served::request &req)
            {
                json user_info = json::parse(req.body());
                bool validation = Registere::userValidation(user_info, res);
                if (!validation)
                {
                    return;
                }
                std::string user_name = user_info["userName"];
                std::string password = user_info["password"];

                bool userAdded = Registere::addUser(user_name, password);
                if (!userAdded)
                {
                    res << "error while adding into db";
                    return;
                }
                // res.set_header("content-type","application/json");
                res << "Registered " << user_name << " successfull";
            };
        }

        auto UpdateHandler()
        {
            return [&](served::response &res, const served::request &req)
            {
                res << "hi, i am UPDATE endpoint";
            };
        }

        auto DeleteHandler()
        {
            return [&](served::response &res, const served::request &req)
            {
                json user_info = json::parse(req.body());
                bool validation = Registere::userValidation(user_info, res);
                if (!validation)
                {
                    return;
                }
                std::string user_name = user_info["userName"];
                std::string password = user_info["password"];

                if (!deleteHandle::deleteuser(user_name, password,res))
                {
                    return;
                }
                res << "user " << user_name << " removed ";
                // res.set_header("content-type", "application/json");
                // res << req.body();
            };
        }

        auto GetAllHandler()
        {
            return [&](served::response &res, const served::request &req)
            {
                try
                {
                    result = stmt->executeQuery("SELECT username FROM userlogin");
                }
                catch (sql::SQLException e)
                {
                    std::cout << "Error message: " << e.what() << std::endl;
                }
                std::vector<std::string> users;
                while (result->next())
                {
                    std::string user = result->getString(1);
                    users.push_back(user);
                }

                json users_userName = users;
                res.set_header("content-type", "application/json");
                res << users_userName.dump();
            };
        }

        void EndpointHandler()
        {
            mux.handle(Register).post(RegisterHandler());
            mux.handle(UpdateEndpoint).post(UpdateHandler());
            mux.handle(DeleteEndpoint).post(DeleteHandler());
            mux.handle(GetAllEndpoint).get(GetAllHandler());
        }

        void StartServer()
        {
            std::cout << "server listioning at " << Port << std::endl;
            served::net::server server(IpAddress, Port, mux);
            server.run(ThreadCount);
        }

    private:
        served::multiplexer mux;
    };

} // EndpointsMethods

int main()
{
    db::dbConnection();
    served::multiplexer mux_;
    EndpointMethods::Httpserver http_server(mux_);
    http_server.EndpointHandler();
    http_server.StartServer();
}