#include "command_response.hpp"
#include "metasurface/pattern_catalog.hpp"
#include "metasurface/text_output.hpp"
#include <string.h>

namespace metasurface {
namespace {

void write_text(TextOutput& output, const char* text) {
    output.write(text, strlen(text));
}

void write_number(TextOutput& output, int number) {
    char digits[2];
    size_t length = 0;
    if (number >= 10) {
        digits[length++] = static_cast<char>('0' + number / 10);
    }
    digits[length++] = static_cast<char>('0' + number % 10);
    output.write(digits, length);
}

}

void write_command_response(CommandResponse response, int mode, TextOutput* output) {
    if (output == nullptr) {
        return;
    }

    switch (response) {
    case CommandResponse::ModeSelected:
        write_text(*output, "OK: modo M");
        write_number(*output, mode);
        write_text(*output, "; saidas desligadas.\n");
        break;
    case CommandResponse::TableApplied:
        write_text(*output, "OK: tabela aplicada.\n");
        break;
    case CommandResponse::OutputsApplied:
        write_text(*output, "OK: saidas atualizadas.\n");
        break;
    case CommandResponse::ModeRequired:
        write_text(*output, "ERRO: selecione um modo de M1 a M32.\n");
        break;
    case CommandResponse::InvalidMode:
        write_text(*output, "ERRO: modo invalido; use M1 a M32.\n");
        break;
    case CommandResponse::InvalidTable:
        write_text(*output, "ERRO: tabela invalida; use T1 a T");
        write_number(*output, pattern_count(mode));
        write_text(*output, ".\n");
        break;
    case CommandResponse::InvalidBinary:
        write_text(*output, "ERRO: entrada binaria invalida; envie exatamente ");
        write_number(*output, mode);
        write_text(*output, mode == 1 ? " bit (0 ou 1).\n" : " bits (0 ou 1).\n");
        break;
    case CommandResponse::InvalidCommand:
        write_text(*output, "ERRO: comando invalido; use M1 a M32, Tn, LISTA ou bits.\n");
        break;
    case CommandResponse::InvalidLine:
        write_text(*output, "ERRO: linha invalida; use ate 32 caracteres e termine com LF ou CRLF.\n");
        break;
    }
}

}
