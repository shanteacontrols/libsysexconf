/*
    Copyright 2017-2022 Igor Petrovic

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <vector>
#include <inttypes.h>
#include <stdlib.h>

namespace lib::sysexconf
{
    ///
    /// \brief Descriptive list of SysEx wish bytes.
    ///
    enum class wish_t : uint8_t
    {
        GET,
        SET,
        BACKUP,
        INVALID
    };

    ///
    /// \brief Descriptive list of SysEx amount bytes.
    ///
    enum class amount_t : uint8_t
    {
        SINGLE,
        ALL,
        INVALID
    };

    ///
    /// \brief Descriptive list of possible SysEx message statuses.
    ///
    enum class status_t : uint8_t
    {
        REQUEST,                 // 0x00
        ACK,                     // 0x01
        ERROR_STATUS,            // 0x02
        ERROR_CONNECTION,        // 0x03
        ERROR_WISH,              // 0x04
        ERROR_AMOUNT,            // 0x05
        ERROR_BLOCK,             // 0x06
        ERROR_SECTION,           // 0x07
        ERROR_PART,              // 0x08
        ERROR_INDEX,             // 0x09
        ERROR_NEW_VALUE,         // 0x0A
        ERROR_MESSAGE_LENGTH,    // 0x0B
        ERROR_WRITE,             // 0x0C
        ERROR_NOT_SUPPORTED,     // 0x0D
        ERROR_READ,              // 0x0E
    };

    ///
    /// \brief List of special SysEx IDs.
    ///
    enum class specialRequest_t : uint8_t
    {
        CONN_CLOSE,            // 0x00
        CONN_OPEN,             // 0x01
        BYTES_PER_VALUE,       // 0x02
        PARAMS_PER_MESSAGE,    // 0x03
        AMOUNT
    };

    ///
    /// \brief Descriptive list of bytes in SysEx message.
    ///
    enum class byteOrder_t : uint8_t
    {
        START_BYTE,      // 0
        ID_BYTE_1,       // 1
        ID_BYTE_2,       // 2
        ID_BYTE_3,       // 3
        STATUS_BYTE,     // 4
        PART_BYTE,       // 5
        WISH_BYTE,       // 6
        AMOUNT_BYTE,     // 7
        BLOCK_BYTE,      // 8
        SECTION_BYTE,    // 9
        INDEX_BYTE,      // 10
    };

    constexpr uint16_t PARAMS_PER_MESSAGE   = 32;
    constexpr uint16_t BYTES_PER_VALUE      = 2;
    constexpr uint8_t  SPECIAL_REQ_MSG_SIZE = (static_cast<uint8_t>(byteOrder_t::WISH_BYTE) + 1) + 1;    // extra byte for end
    constexpr uint8_t  STD_REQ_MIN_MSG_SIZE = static_cast<uint8_t>(byteOrder_t::INDEX_BYTE) + (BYTES_PER_VALUE * 2) + 1;
    constexpr uint16_t MAX_MESSAGE_SIZE     = STD_REQ_MIN_MSG_SIZE + (PARAMS_PER_MESSAGE * BYTES_PER_VALUE);

    ///
    /// \brief Structure holding SysEx manufacturer ID bytes.
    ///
    struct ManufacturerId
    {
        uint8_t id1 = 0;
        uint8_t id2 = 0;
        uint8_t id3 = 0;
    };

    ///
    /// \brief Structure containing data for single custom request.
    ///
    struct CustomRequest
    {
        uint16_t requestId     = 0;        ///< ID byte representing specific request.
        bool     connOpenCheck = false;    ///< Flag indicating whether or not SysEx connection should be enabled before processing request.
    };

    ///
    /// \brief Structure holding decoded request data.
    ///
    struct DecodedMessage
    {
        status_t status   = {};
        wish_t   wish     = {};
        amount_t amount   = {};
        uint8_t  block    = 0;
        uint8_t  section  = 0;
        uint8_t  part     = 0;
        uint16_t index    = 0;
        uint16_t newValue = 0;
    };

    class Section
    {
        public:
        Section(
            uint16_t numberOfParameters,
            uint16_t newValueMin,
            uint16_t newValueMax)
            : NUMBER_OF_PARAMETERS(numberOfParameters)
            , NEW_VALUE_MIN(newValueMin)
            , NEW_VALUE_MAX(newValueMax)
        {
            // based on number of parameters, calculate how many parts message has in case of set/all request and get/all response
            _parts = numberOfParameters / PARAMS_PER_MESSAGE;

            if (numberOfParameters % PARAMS_PER_MESSAGE)
            {
                _parts++;
            }
        }

        uint16_t numberOfParameters() const
        {
            return NUMBER_OF_PARAMETERS;
        }

        uint16_t newValueMin() const
        {
            return NEW_VALUE_MIN;
        }

        uint16_t newValueMax() const
        {
            return NEW_VALUE_MAX;
        }

        uint8_t parts() const
        {
            return _parts;
        }

        private:
        const uint16_t NUMBER_OF_PARAMETERS;
        const uint16_t NEW_VALUE_MIN;
        const uint16_t NEW_VALUE_MAX;
        uint8_t        _parts = 0;
    };

    ///
    /// \brief Structure holding data for single SysEx block.
    ///
    class Block
    {
        public:
        Block(std::vector<Section>& sections)
            : _sections(sections)
        {}

        private:
        friend class SysExConf;
        std::vector<Section>& _sections;
    };

    class Merge14Bit
    {
        public:
        Merge14Bit(uint8_t high, uint8_t low)
        {
            if (high & 0x01)
            {
                low |= (1 << 7);
            }
            else
            {
                low &= ~(1 << 7);
            }

            high >>= 1;

            uint16_t joined = 0;
            joined          = high;
            joined <<= 8;
            joined |= low;

            _value = joined;
        }

        uint16_t value() const
        {
            return _value;
        }

        private:
        uint16_t _value = 0;
    };

    ///
    /// \brief Helper class used to convert single 14-bit value to high and low bytes (7-bit each).
    /// @param [in]     value   14-bit value to split.
    ///
    class Split14Bit
    {
        public:
        Split14Bit(uint16_t value)
        {
            uint8_t newHigh = (value >> 8) & 0xFF;
            uint8_t newLow  = value & 0xFF;
            newHigh         = (newHigh << 1) & 0x7F;

            if ((newLow >> 7) & 0x01)
            {
                newHigh |= 0x01;
            }
            else
            {
                newHigh &= ~0x01;
            }

            newLow &= 0x7F;
            _high = newHigh;
            _low  = newLow;
        }

        uint8_t high() const
        {
            return _high;
        }

        uint8_t low() const
        {
            return _low;
        }

        private:
        uint8_t _high = 0;
        uint8_t _low  = 0;
    };

    class DataHandler
    {
        public:
        class CustomResponse
        {
            public:
            CustomResponse(uint8_t* responseArray, uint16_t& responseCounter)
                : _responseArray(responseArray)
                , _responseCounter(responseCounter)
            {}

            void append(uint16_t value)
            {
                value &= 0x3FFF;

                // make sure to leave space for 0xF7 byte
                if ((_responseCounter - 2) < MAX_MESSAGE_SIZE)
                {
                    // split into two 7-bit values

                    auto split                         = Split14Bit(value);
                    _responseArray[_responseCounter++] = split.high();
                    _responseArray[_responseCounter++] = split.low();
                }
            }

            private:
            uint8_t*  _responseArray;
            uint16_t& _responseCounter;
        };

        DataHandler() = default;

        virtual uint8_t get(uint8_t block, uint8_t section, uint16_t index, uint16_t& value)   = 0;
        virtual uint8_t set(uint8_t block, uint8_t section, uint16_t index, uint16_t newValue) = 0;
        virtual uint8_t customRequest(uint16_t request, CustomResponse& customResponse)        = 0;
        virtual void    sendResponse(uint8_t* array, uint16_t size)                            = 0;
    };
}    // namespace lib::sysexconf