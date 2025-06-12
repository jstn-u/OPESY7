#pragma once

class ICommand {
public:
    enum CommandType {
        PRINT
    };

    ICommand(CommandType type);
    CommandType getType();
    virtual void execute() = 0;

    protected:
        CommandType type;
};