#include <iostream>
#include <string>

struct InputBuffer
{
    std::string buffer;
};

InputBuffer *new_input_buffer()
{
    InputBuffer *input_buffer = new InputBuffer;
    return input_buffer;
}

void print_prompt()
{
    std::cout << "db > ";
}

void read_input(InputBuffer *input_buffer)
{
    std::getline(std::cin, input_buffer->buffer);

    if (input_buffer->buffer.size() <= 0)
    {
        std::cout << "Error reading input" << std::endl;
        exit(EXIT_FAILURE);
    }
}

void close_input_buffer(InputBuffer *input_buffer)
{
    delete input_buffer;
}

int main(int argc, char *argv[])
{
    InputBuffer *input_buffer = new_input_buffer();
    while (true)
    {
        print_prompt();
        read_input(input_buffer);
        if (input_buffer->buffer == ".exit")
        {
            close_input_buffer(input_buffer);
            exit(EXIT_SUCCESS);
        }
        else
        {
            std::cout << "Unrecognized command '" << input_buffer->buffer << "'." << std::endl;
        }
    }
    return 0;
}
