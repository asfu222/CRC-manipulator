#include <cstdio>
#include "file.h"
#include <stdexcept>

std::unique_ptr<File> File::fromFileHandle(FILE *fileHandle)
{
    return std::unique_ptr<File>(new File(fileHandle));
}

std::unique_ptr<File> File::fromFileName(const std::string &fileName, int mode)
{
    std::string modeString;

    if ((mode & Mode::Write) && (mode & Mode::Read))
        modeString += "a+";
    else if (mode & Mode::Write)
        modeString += "w";
    else if (mode & Mode::Read)
        modeString += "r";

    if (mode & Mode::Binary)
        modeString += "b";

    FILE *fileHandle = fopen(fileName.c_str(), modeString.c_str());
    if (fileHandle == nullptr)
    {
        throw std::runtime_error("Couldn't open file for " +
            std::string(mode & Mode::Write ? "writing" : "reading"));
    }
    return fromFileHandle(fileHandle);
}

File::File(FILE *fileHandle) : fileHandle(fileHandle)
{
    try
    {
        seek(0, Origin::End);
        fileSize = tell();
        seek(0, Origin::Start);
    }
    catch (...)
    {
        fileSize = -1;
    }
}

File::~File()
{
    fclose(fileHandle);
}

File &File::seek(OffsetType offset, Origin origin)
{
    if (getSize() == -1)
        throw std::runtime_error("Stream is unseekable");

    int type;

    OffsetType destination = 0;
    switch (origin)
    {
        case Origin::Ahead:
            destination = offset;
            type = SEEK_CUR;
            break;

        case Origin::Behind:
            destination = -offset;
            type = SEEK_CUR;
            break;

        case Origin::Start:
            destination = offset;
            type = SEEK_SET;
            break;

        case Origin::End:
            destination = offset;
            type = SEEK_END;
            break;

        default:
            throw std::invalid_argument("Bad offset type");
    }

    #if HAVE_FSEEKO64
        auto ret = fseeko64(fileHandle, destination, type);
    #elif HAVE_FSEEKI64
        auto ret = _fseeki64(fileHandle, destination, type);
    #elif HAVE_FSEEKO
        auto ret = fseeko(fileHandle, destination, type);
    #else
        auto ret = fseek(fileHandle, destination, type);
    #endif
    if (ret != 0)
        throw std::runtime_error("Stream is unseekable");
    return *this;
}

File::OffsetType File::tell() const
{
    #if HAVE_FSEEKO64
        auto ret = ftello64(fileHandle);
    #elif HAVE_FSEEKI64
        auto ret = _ftelli64(fileHandle);
    #elif HAVE_FSEEKO
        auto ret = ftello(fileHandle);
    #else
        auto ret = ftell(fileHandle);
    #endif
    if (ret == -1L)
        throw std::runtime_error("Stream is unseekable");
    return ret;
}

File &File::read(char *buffer, size_t size)
{
    return read(reinterpret_cast<unsigned char*>(buffer), size);
}

File &File::read(unsigned char *buffer, size_t size)
{
    OffsetType newPos = tell() + static_cast<OffsetType>(size);

    if (newPos > getSize())
        throw std::runtime_error("Trying to read content beyond EOF");

    if (fread(buffer, sizeof(unsigned char), size, fileHandle) != size)
    {
        throw std::runtime_error("Can't read bytes at "
            + std::to_string(tell()));
    }

    return *this;
}

File &File::write(const char *buffer, size_t size)
{
    return write(reinterpret_cast<const unsigned char*>(buffer), size);
}

File &File::write(const unsigned char *buffer, size_t size)
{
    if (fwrite(buffer, sizeof(unsigned char), size, fileHandle) != size)
        throw std::runtime_error("Can't write bytes");

    if (fileSize >= 0 && fileSize < tell())
        fileSize = tell();

    return *this;
}

File::OffsetType File::getSize() const
{
    return fileSize;
}
