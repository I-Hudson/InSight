#include "Serialisation/Serialisers/BinarySerialiser.h"

#include "Platforms/Platform.h"
#include "Core/Memory.h"
#include "Core/Logger.h"

namespace Insight
{
    namespace Serialisation
    {
        BinaryHead::BinaryHead()
        {
            Resize(1024);
        }

        BinaryHead::~BinaryHead()
        {
            if (Data)
            {
                Clear();
                Capacity = 0;
                DeleteBytes(Data);
            }
        }

        void BinaryHead::Write(std::string_view tag, const void* data, u64 sizeBytes)
        {
            const u64 requiredSize = Size + sizeBytes;
            if (requiredSize > Capacity)
            {
                Resize(AlignUp(requiredSize, 4));
            }
            Platform::MemCopy(Data + Size, data, sizeBytes);
            Size += sizeBytes;
        }

        void BinaryHead::Read(std::string_view tag, const void* data, u64 sizeBytes)
        {
            Platform::MemCopy((void*)data, Data + Size, sizeBytes);
            ASSERT(Size + sizeBytes <= Capacity);
            Size += sizeBytes;
        }

        void BinaryHead::Resize(u64 newSize)
        {
            if (newSize <= Capacity)
            {
                return;
            }

            Byte* newBlock = static_cast<Byte*>(NewBytes(newSize));
            Platform::MemSet(newBlock, 0, newSize);
            if (Data)
            {
                Platform::MemCopy(newBlock, Data, Capacity);
                Delete(Data);
            }
            Data = newBlock;
            Capacity = newSize;
        }

        void BinaryHead::PushState(SerialiserNodeStates state)
        {
            NodeStates.push(state);
        }

        void BinaryHead::PopState()
        {
            NodeStates.pop();
        }

        SerialiserNodeStates BinaryHead::GetCurrentState() const
        {
            return NodeStates.top();
        }

        void BinaryHead::Clear()
        {
            Size = 0;
        }

        void BinaryHead::Deserialise(const std::vector<Byte>& data)
        {
            Resize(data.size());
            Platform::MemCopy(Data, data.data(), data.size());
            Clear();
        }


        BinarySerialiser::BinarySerialiser()
            : ISerialiser(SerialisationTypes::Binary, false)
        {
            m_head.Clear();
        }

        BinarySerialiser::BinarySerialiser(bool isReadMode)
            : ISerialiser(SerialisationTypes::Binary, isReadMode)
        {
            m_head.Clear();
        }

        BinarySerialiser::~BinarySerialiser()
        {
            m_head.Clear();
        }

        bool BinarySerialiser::Deserialise(std::vector<u8> data)
        {
            ReadType(data);
            m_head.Deserialise(data);
            return true;
        }

        std::vector<Byte> BinarySerialiser::GetSerialisedData()
        {
            WriteType();

            std::vector<Byte> serialisedData;
            serialisedData.resize(m_head.Size);
            Platform::MemCopy(serialisedData.data(), m_head.Data, m_head.Size);

            return serialisedData;
        }

        void BinarySerialiser::Clear()
        {
            m_head.Clear();
        }

        void BinarySerialiser::StartObject(std::string_view name)
        {
            m_head.PushState(SerialiserNodeStates::Object);
        }

        void BinarySerialiser::StopObject()
        {
            ASSERT(IsObjectNode());
            m_head.PopState();
        }

        void BinarySerialiser::StartArray(std::string_view name, u64& size)
        {
            m_head.PushState(SerialiserNodeStates::Array);
            if (IsReadMode())
            {
                Read(name, size);
            }
            else
            {
                Write(name, size);
            }
        }

        void BinarySerialiser::StopArray()
        {
            ASSERT(IsArrayNode());
            m_head.PopState();
        }

        void BinarySerialiser::Write(std::string_view tag, bool data)
        {
            Write<bool>(tag, data);
        }

        void BinarySerialiser::Write(std::string_view tag, char data)
        {
            Write<char>(tag, data);
        }

        void BinarySerialiser::Write(std::string_view tag, float data)
        {
            Write<float>(tag, data);
        }

        void BinarySerialiser::Write(std::string_view tag, u8 data)
        {
            Write<u8>(tag, data);
        }
        void BinarySerialiser::Write(std::string_view tag, u16 data)
        {
            Write<u16>(tag, data);
        }
        void BinarySerialiser::Write(std::string_view tag, u32 data)
        {
            Write<u32>(tag, data);
        }
        void BinarySerialiser::Write(std::string_view tag, u64 data)
        {
            Write<u64>(tag, data);
        }

        void BinarySerialiser::Write(std::string_view tag, i8 data)
        {
            Write<i8>(tag, data);
        }
        void BinarySerialiser::Write(std::string_view tag, i16 data)
        {
            Write<i16>(tag, data);
        }
        void BinarySerialiser::Write(std::string_view tag, i32 data)
        {
            Write<i32>(tag, data);
        }
        void BinarySerialiser::Write(std::string_view tag, i64 data)
        {
            Write<i64>(tag, data);
        }

        void BinarySerialiser::Write(std::string_view tag, std::string const& string)
        {
            u64 arraySize = string.size();
            StartArray(tag, arraySize);
            WriteBlock(tag, string.data(), arraySize);
            StopArray();
        }
        void BinarySerialiser::Write(std::string_view tag, const std::vector<Byte>& vector)
        {
            u64 arraySize = vector.size();
            StartArray(tag, arraySize);
            WriteBlock(tag, vector.data(), arraySize);
            /*for (size_t i = 0; i < arraySize; ++i)
            {
                Byte c = vector.at(i);
                Write(tag, c);
            }*/
            StopArray();
        }

        void BinarySerialiser::Read(std::string_view tag, bool& data)
        {
            ReadValue<bool>(tag, data);
        }

        void BinarySerialiser::Read(std::string_view tag, char& data)
        {
            ReadValue<char>(tag, data);
        }

        void BinarySerialiser::Read(std::string_view tag, float& data)
        {
            ReadValue<float>(tag, data);
        }

        void BinarySerialiser::Read(std::string_view tag, u8& data)
        {
            ReadValue<u8>(tag, data);
        }
        void BinarySerialiser::Read(std::string_view tag, u16& data)
        {
            ReadValue<u16>(tag, data);
        }
        void BinarySerialiser::Read(std::string_view tag, u32& data)
        {
            ReadValue<u32>(tag, data);
        }
        void BinarySerialiser::Read(std::string_view tag, u64& data)
        {
            ReadValue<u64>(tag, data);
        }

        void BinarySerialiser::Read(std::string_view tag, i8& data)
        {
            ReadValue<i8>(tag, data);
        }
        void BinarySerialiser::Read(std::string_view tag, i16& data)
        {
            ReadValue<i16>(tag, data);
        }
        void BinarySerialiser::Read(std::string_view tag, i32& data)
        {
            ReadValue<i32>(tag, data);
        }
        void BinarySerialiser::Read(std::string_view tag, i64& data)
        {
            ReadValue<i64>(tag, data);
        }

        void BinarySerialiser::Read(std::string_view tag, std::string& string)
        {
            u64 arraySize = 0;
            StartArray(tag, arraySize);
            string.resize(arraySize);
            ReadBlock(tag, string.data(), arraySize);
            StopArray();
        }
        void BinarySerialiser::Read(std::string_view tag, std::vector<Byte>& vector)
        {
            u64 arraySize = 0;
            StartArray(tag, arraySize);
            vector.resize(arraySize);
            ReadBlock(tag, vector.data(), arraySize);
            StopArray();
        }

        bool BinarySerialiser::ReadType(std::vector<Byte>& data)
        {
            u32 type = 0;
            Platform::MemCopy(&type, &(*(data.end() - 4)), sizeof(u32));

            SerialisationTypes serialiserType = static_cast<SerialisationTypes>(type);
            if (serialiserType != m_type)
            {
                IS_CORE_ERROR("[BinarySerialiser::Deserialise] 'data' has been serialised with type '{}' trying to deserialise with '{}'. Serialiser type mismatch.",
                    SerialisationTypeToString[(u32)type], SerialisationTypeToString[(u32)m_type]);
                return false;
            }
            return true;
        }

        void BinarySerialiser::WriteBlock(std::string_view tag, const void* data, u64 sizeBytes)
        {
            m_head.Write(tag, data, sizeBytes);
        }

        void BinarySerialiser::ReadBlock(std::string_view tag, const void* data, u64 sizeBytes)
        {
            m_head.Read(tag, data, sizeBytes);
        }

        bool BinarySerialiser::IsObjectNode() const
        {
            return m_head.GetCurrentState() == SerialiserNodeStates::Object;
        }

        bool BinarySerialiser::IsArrayNode() const
        {
            return m_head.GetCurrentState() == SerialiserNodeStates::Array;
        }
    }
}