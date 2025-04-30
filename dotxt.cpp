#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include <algorithm>
#include <windows.h>
#include <sstream>

using namespace std;

enum class Command
{
    NONE,
    PRINT,
    COLOR,
    TITLE,
    GOTO,
    PAUSE,
    CLS,
    DELAY,
    SET,
    EXIT
};

struct CommandResult
{
    int nextLine;
    bool shouldContinue;
};

vector<string> SplitString(const string &line)
{
    vector<string> tokens;
    string token;
    istringstream tokenStream(line);

    while (tokenStream >> token)
    {
        tokens.push_back(token);
    }

    return tokens;
}

class Interpreter
{
private:
    vector<string> lines;
    unordered_map<string, char> colorMap = {
        {"black", '0'},
        {"blue", '1'},
        {"green", '2'},
        {"red", '4'},
        {"purple", '5'},
        {"yellow", '6'},
        {"white", '7'},
        {"gray", '8'},
        {"violet", 'D'}};

    unordered_map<string, string> Variables = {};

    Command parseCommand(const string &token)
    {
        if (token == "print")
            return Command::PRINT;
        if (token == "color")
            return Command::COLOR;
        if (token == "title")
            return Command::TITLE;
        if (token == "goto")
            return Command::GOTO;
        if (token == "pause")
            return Command::PAUSE;
        if (token == "cls")
            return Command::CLS;
        if (token == "delay")
            return Command::DELAY;
        if (token == "set")
            return Command::SET;
        if (token == "exit")
            return Command::EXIT;
        return Command::NONE;
    }

    void executeSystemCommand(const string &cmd)
    {
        int result = system(cmd.c_str());
        if (result != 0)
        {
            cerr << "Warning: Command failed - " << cmd << endl;
        }
    }

    CommandResult handlePrint(const string &line, size_t spacePos, int currentLine)
    {
        string message = line.substr(spacePos + 1);
        string output;

        for (size_t i = 0; i < message.size();)
        {
            if (message[i] == '$' && (i + 1 < message.size()))
            {
                string varName;
                size_t j = i + 1;

                while (j < message.size() &&
                       (isalnum(message[j]) || message[j] == '_'))
                {
                    varName += message[j++];
                }

                if (Variables.count(varName))
                {
                    output += Variables[varName];
                }
                else
                {
                    output += "$" + varName;
                    cerr << "Warn: Variable " << varName << " does not exist" << endl;
                }

                i = j;
            }
            else
            {
                output += message[i++];
            }
        }

        cout << output << endl;
        return {currentLine + 1, true};
    }

    CommandResult handleColor(const string &line, size_t spacePos, int currentLine)
    {
        string color = line.substr(spacePos + 1);
        transform(color.begin(), color.end(), color.begin(), ::tolower);

        auto it = colorMap.find(color);
        if (it != colorMap.end())
        {
            executeSystemCommand("color " + string(1, it->second));
        }
        else
        {
            cerr << "Error: Unknown color '" << color << "'" << endl;
            return {currentLine + 1, false};
        }
        return {currentLine + 1, true};
    }

    CommandResult handleTitle(const string &line, size_t spacePos, int currentLine)
    {
        string title = line.substr(spacePos + 1);
        executeSystemCommand("title " + title);
        return {currentLine + 1, true};
    }

    CommandResult handleGoto(const string &line, size_t spacePos, int currentLine)
    {
        string target = line.substr(spacePos + 1);
        try
        {
            int lineNumber = stoi(target) - 1;
            if (lineNumber < 0 || lineNumber >= static_cast<int>(lines.size()))
            {
                throw out_of_range("Line number out of range");
                return {currentLine + 1, false};
            }
            return {lineNumber, true};
        }
        catch (const invalid_argument &)
        {
            cerr << "Error: Invalid line number '" << target << "'" << endl;
            return {currentLine + 1, false};
        }
        catch (const out_of_range &)
        {
            cerr << "Error: Line number " << target << " is out of range" << endl;
            return {currentLine + 1, false};
        }
        return {currentLine + 1, true};
    }

    CommandResult handlePause(int currentLine)
    {
        executeSystemCommand("pause");
        return {currentLine + 1, true};
    }

    CommandResult handleCls(int currentLine)
    {
        executeSystemCommand("cls");
        return {currentLine + 1, true};
    }

    CommandResult HandleDelay(const string &line, size_t spacePos, int currentLine)
    {

        try
        {
            int Time = stoi(line.substr(spacePos + 1));

            if (Time < 0)
            {
                cerr << "Error: Delay value cannot be negative" << endl;
                return {currentLine + 1, false};
            }

            Sleep(Time);
        }
        catch (const invalid_argument)
        {
            cerr << "Error: Invalid delay value" << endl;
            return {currentLine + 1, false};
        }
        catch (const out_of_range)
        {
            cerr << "Error: Delay value is out of range" << endl;
            return {currentLine + 1, false};
        }

        return {currentLine + 1, true};
    }

    CommandResult HandleSet(const string &line, size_t spacePos, int currentLine)
    {
        try
        {
            string content = line.substr(spacePos + 1);

            size_t equalPos = content.find('=');
            if (equalPos == string::npos)
            {
                cerr << "Error: Missing \"=\". Use: set var = value" << endl;
                return {currentLine + 1, false};
            }

            string varName = content.substr(0, equalPos);

            varName.erase(remove_if(varName.begin(), varName.end(), [](char c){ return isspace(c);}), varName.end());

            string value = content.substr(equalPos + 1);

            if (!value.empty() && value[0] == ' ')
            {
                value = value.substr(1);
            }

            Variables[varName] = value;
        }
        catch (const invalid_argument)
        {
            cerr << "Error: Invalid variable" << endl;
            return {currentLine + 1, false};
        }
        catch (const out_of_range)
        {
            cerr << "Error: Variable out of range" << endl;
            return {currentLine + 1, false};
        }

        return {currentLine + 1, true};
    }

    CommandResult HandleExit(int currentLine)
    {
        exit(0);
        return {currentLine + 1, false};
    }

public:
    Interpreter(const string &filename)
    {
        ifstream file(filename);
        if (!file.is_open())
        {
            throw runtime_error("Could not open file: " + filename);
        }

        string line;
        while (getline(file, line))
        {
            if (!line.empty())
            {
                lines.push_back(line);
            }
        }
        file.close();
    }

    void run()
    {
        for (int i = 0; i < static_cast<int>(lines.size());)
        {
            const string &line = lines[i];
            size_t spacePos = line.find(' ');
            string commandToken = spacePos == string::npos ? line : line.substr(0, spacePos);

            Command cmd = parseCommand(commandToken);
            CommandResult result;

            try
            {
                switch (cmd)
                {
                case Command::PRINT:
                    result = handlePrint(line, spacePos, i);
                    break;
                case Command::COLOR:
                    result = handleColor(line, spacePos, i);
                    break;
                case Command::TITLE:
                    result = handleTitle(line, spacePos, i);
                    break;
                case Command::GOTO:
                    result = handleGoto(line, spacePos, i);
                    break;
                case Command::PAUSE:
                    result = handlePause(i);
                    break;
                case Command::CLS:
                    result = handleCls(i);
                    break;
                case Command::DELAY:
                    result = HandleDelay(line, spacePos, i);
                    break;
                case Command::SET:
                    result = HandleSet(line, spacePos, i);
                    break;
                case Command::EXIT:
                    result = HandleExit(i);
                    break;
                default:
                    cerr << "Error: Unknown command '" << commandToken << "'" << endl;
                    result = {i + 1, true};
                    break;
                }
            }
            catch (const exception &e)
            {
                cerr << "Error executing command: " << e.what() << endl;
                result = {i + 1, true};
            }

            if (!result.shouldContinue)
            {
                system("pause");
                break;
            }
            i = result.nextLine;
        }
    }
};

int main()
{
    try
    {
        Interpreter interpreter("program.txt");
        interpreter.run();
    }
    catch (const exception &e)
    {
        cerr << "Fatal error: " << e.what() << endl;
        return 1;
    }
    return 0;
}
