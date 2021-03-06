/*************************************************************************
 * Copyright (c) 2015, Dave Brown
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR *CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL *DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 ************************************************************************/
#include <cantools/parsers/dbc_parser.h>

#include <sstream>
#include <utility>

#include <iostream>

namespace dbc
{
    namespace
    {
        std::string const MESSAGE_TYPE = "BO_ ";
        std::string const SIGNAL_TYPE = " SG_ ";
    }

    dbc_file parse_dbc(std::stringstream && stream)
    {
        dbc_file result;
        std::string line;
        while(std::getline(stream, line))
        {
            if(line.substr(0, 4) == MESSAGE_TYPE)
            {
                // Trim off the BO_ noise
                line.erase(0, 4);
                auto message = on_parse_message(line);
                result.messages.push_back(message);
            }
            else if(line.substr(0, 5) == SIGNAL_TYPE)
            {
                // Trim off the SG_ noise
                line.erase(0, 5);
                auto signal = on_parse_signal(line);
                result.messages.back().signals.push_back(signal);
            }
            else
            {
                // Do nothing with attributes and value tables for now...
            }
        }
        return result;
    }

    dbc_message on_parse_message(std::string message_to_parse)
    {
        dbc_message result;
        std::stringstream message_stream(message_to_parse);
        message_stream >> result.id;
        message_stream >> result.name;
        message_stream >> result.length;
        message_stream >> result.sender;

        // Quickly trim off the colon at the end of the name...
        result.name.erase(result.name.end()-1);
        return result;
    }

    dbc_signal on_parse_signal(std::string signal_to_parse)
    {
        dbc_signal result;
        std::stringstream signal_stream(signal_to_parse);
        signal_stream >> result.name;
        auto bit_start_and_length = detail::extract_bit_start_and_length_from_string(signal_to_parse);
        result.start_bit = bit_start_and_length.first;
        result.bit_length = bit_start_and_length.second;

        result.endianness = detail::extract_endianness_from_string(signal_to_parse);
        result.signedness = detail::extract_signedness_from_string(signal_to_parse);

        auto scale_and_offset = detail::extract_scale_and_offset_from_string(signal_to_parse);
        result.scale = scale_and_offset.first;
        result.offset = scale_and_offset.second;

        auto min_and_max = detail::extract_min_and_max_from_string(signal_to_parse);
        result.min = min_and_max.first;
        result.max = min_and_max.second;

        result.unit = detail::extract_unit_from_string(signal_to_parse);
        return result;
    }

    namespace detail
    {
        std::pair< unsigned int, unsigned int > extract_bit_start_and_length_from_string(std::string signal)
        {
            auto string_start = signal.find(":");
            auto string_end   = signal.find("@");
            auto start_bit_and_length_string = signal.substr(string_start, string_end - string_start);

            auto separator = start_bit_and_length_string.find("|");

            auto start_bit_value = start_bit_and_length_string.substr(1, separator - 1);
            auto bit_length_value = start_bit_and_length_string.substr(separator + 1);

            return std::make_pair(std::stoi(start_bit_value), std::stoi(bit_length_value));
        }

        Endianness extract_endianness_from_string(std::string signal)
        {
            auto endianness_location = signal.find("@");
            auto endianness_value = std::stoi(signal.substr(endianness_location + 1, 1));

            return endianness_value == 0 ? CAN_BIG_ENDIAN : CAN_LITTLE_ENDIAN;
        }

        Signedness extract_signedness_from_string(std::string signal)
        {
            auto signedness_location = signal.find("@") + 2;
            auto signedness_value = signal.substr(signedness_location, 1);
            return signedness_value == "+" ? UNSIGNED : SIGNED;
        }

        std::pair< double, double > extract_scale_and_offset_from_string(std::string signal)
        {
            auto string_start = signal.find("(");
            auto string_end   = signal.find(")");
            auto scale_and_offset_string = signal.substr(string_start, string_end - string_start);

            auto separator = scale_and_offset_string.find(",");

            auto scale_value = scale_and_offset_string.substr(1, separator - 1);
            auto offset_value = scale_and_offset_string.substr(separator + 1);
            return std::make_pair(std::stod(scale_value), std::stod(offset_value));
        }

        std::pair< double, double > extract_min_and_max_from_string(std::string signal)
        {
            auto string_start = signal.find("[");
            auto string_end   = signal.find("]");
            auto min_and_max_string = signal.substr(string_start, string_end - string_start);

            auto separator = min_and_max_string.find("|");

            auto min_value = min_and_max_string.substr(1, separator - 1);
            auto max_value = min_and_max_string.substr(separator + 1);
            return std::make_pair(std::stod(min_value), std::stod(max_value));
        }

        std::string extract_unit_from_string(std::string signal)
        {
            auto unit_start_location = signal.find("\"");
            auto unit_end_location = signal.find("\"", unit_start_location + 1);
            auto unit_string = signal.substr(unit_start_location + 1, unit_end_location - unit_start_location - 1);
            return unit_string;
        }
    }
}
