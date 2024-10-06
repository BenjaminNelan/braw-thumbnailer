/*
 ** ------------------------
 ** BRAW Thumbnailer
 ** ------------------------
 ** Based on `ExtractFrame` example from Blackmagic RAW SDK
 ** https://www.blackmagicdesign.com/au/products/blackmagicraw
 */

#include "BlackmagicRawAPI.h"
#include <Magick++.h>
#include <sys/stat.h>
#include <iostream>
#include <cstring>
#include <string>
#include <filesystem>

#ifdef DEBUG
    #include <cassert>
    #define VERIFY(condition) assert(SUCCEEDED(condition))
#else
    #define VERIFY(condition) condition
#endif

static const BlackmagicRawResourceFormat s_resourceFormat = blackmagicRawResourceFormatRGBAU8;

void GenerateThumbnail(unsigned int width, unsigned int height, void* imageData, const std::string& outputPath, int size)
{
    try {
        Magick::Image image(width, height, "RGBA", Magick::CharPixel, imageData);

        // Resize to max width while retaining aspect ratio.
        int newWidth = width;
        int newHeight = height;
        if (width > size || height > size) {
            if (width > height) {
                newWidth = size;
                newHeight = (height * size) / width;
            } else {
                newHeight = size;
                newWidth = (width * size) / height;
            }
        }

        image.resize(Magick::Geometry(newWidth, newHeight));
        image.magick("PNG");
        image.write(outputPath);

        std::cout << "BRAWThumbnailer: Thumbnail saved as " << outputPath << std::endl;
    }
    catch (Magick::Exception &error) {
        std::cerr << "BRAWThumbnailer: Error: " << error.what() << std::endl;
    }
}

bool IsDir(const std::string& path)
{
    struct stat info;
    if (stat(path.c_str(), &info) != 0) { return false; }
    if (info.st_mode & S_IFDIR) { return true; }
    return false;
}

const char* GetLibraryPath()
{
    // Note: It seems like only /usr/lib directories are supported by Nautilus if using Nemo/Dolphin you can use /opt/resolve/libs
    static const char* lib_path = "/usr/lib/blackmagic/BlackmagicRAWSDK/Linux/Libraries";
    static const char* lib64_path = "/usr/lib64/blackmagic/BlackmagicRAWSDK/Linux/Libraries";
    
    if (!IsDir(std::string(lib_path))) {
        return lib64_path;
    }
    return lib_path;
}

class CameraCodecCallback : public IBlackmagicRawCallback
{
public:
    explicit CameraCodecCallback(const std::string& outputPath, int size) 
        : m_outputPath(outputPath), m_size(size) {}
    virtual ~CameraCodecCallback() = default;

    virtual void ReadComplete(IBlackmagicRawJob* readJob, HRESULT result, IBlackmagicRawFrame* frame)
    {
        IBlackmagicRawJob* decodeAndProcessJob = nullptr;

        if (result == S_OK)
            VERIFY(frame->SetResourceFormat(s_resourceFormat));

        if (result == S_OK)
            result = frame->CreateJobDecodeAndProcessFrame(nullptr, nullptr, &decodeAndProcessJob);

        if (result == S_OK)
            result = decodeAndProcessJob->Submit();

        if (result != S_OK)
        {
            if (decodeAndProcessJob)
                decodeAndProcessJob->Release();
        }

        readJob->Release();
    }

    virtual void ProcessComplete(IBlackmagicRawJob* job, HRESULT result, IBlackmagicRawProcessedImage* processedImage)
    {
        unsigned int width = 0;
        unsigned int height = 0;
        void* imageData = nullptr;

        if (result == S_OK)
            result = processedImage->GetWidth(&width);

        if (result == S_OK)
            result = processedImage->GetHeight(&height);

        if (result == S_OK)
            result = processedImage->GetResource(&imageData);

        if (result == S_OK)
            GenerateThumbnail(width, height, imageData, m_outputPath, m_size);

        job->Release();
    }

	virtual void DecodeComplete(IBlackmagicRawJob*, HRESULT) {}
	virtual void TrimProgress(IBlackmagicRawJob*, float) {}
	virtual void TrimComplete(IBlackmagicRawJob*, HRESULT) {}
	virtual void SidecarMetadataParseWarning(IBlackmagicRawClip*, const char*, uint32_t, const char*) {}
	virtual void SidecarMetadataParseError(IBlackmagicRawClip*, const char*, uint32_t, const char*) {}
	virtual void PreparePipelineComplete(void*, HRESULT) {}

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID*)
	{
		return E_NOTIMPL;
	}

	virtual ULONG STDMETHODCALLTYPE AddRef(void)
	{
		return 0;
	}

	virtual ULONG STDMETHODCALLTYPE Release(void)
	{
		return 0;
	}
private:
    std::string m_outputPath;
    int m_size;
};

int main(int argc, const char* argv[])
{
    if (argc != 4)
    {
        std::cerr << "BRAWThumbnailer (v0.1) Usage: " << argv[0] << " <input_file.braw> <output_file.png> <size>" << std::endl;
        return 1;
    }

    const char* inputFile = argv[1];
    const char* outputFile = argv[2];
    int size = std::stoi(argv[3]);

    Magick::InitializeMagick(nullptr);

    HRESULT result = S_OK;

    IBlackmagicRawFactory* factory = nullptr;
    IBlackmagicRaw* codec = nullptr;
    IBlackmagicRawClip* clip = nullptr;
    IBlackmagicRawJob* readJob = nullptr;

    CameraCodecCallback callback(outputFile, size);
    long readTime = 0;

    do
    {
        factory = CreateBlackmagicRawFactoryInstanceFromPath(GetLibraryPath());
        if (factory == nullptr)
        {
            std::cerr << "BRAWThumbnailer: Failed to create IBlackmagicRawFactory!" << std::endl;
            break;
        }

        result = factory->CreateCodec(&codec);
        if (result != S_OK)
        {
            std::cerr << "BRAWThumbnailer: Failed to create IBlackmagicRaw!" << std::endl;
            break;
        }

        result = codec->OpenClip(inputFile, &clip);
        if (result != S_OK)
        {
            std::cerr << "BRAWThumbnailer: Failed to open IBlackmagicRawClip!" << std::endl;
            break;
        }

        result = codec->SetCallback(&callback);
        if (result != S_OK)
        {
            std::cerr << "BRAWThumbnailer: Failed to set IBlackmagicRawCallback!" << std::endl;
            break;
        }

        result = clip->CreateJobReadFrame(readTime, &readJob);
        if (result != S_OK)
        {
            std::cerr << "BRAWThumbnailer: Failed to create IBlackmagicRawJob!" << std::endl;
            break;
        }

        result = readJob->Submit();
        if (result != S_OK)
        {
			// submit has failed, the ReadComplete callback won't be called, release the job here instead
            readJob->Release();
            std::cerr << "BRAWThumbnailer: Failed to submit IBlackmagicRawJob!" << std::endl;
            break;
        }

        codec->FlushJobs();

    } while(0);

    if (clip != nullptr)
        clip->Release();

    if (codec != nullptr)
        codec->Release();

    if (factory != nullptr)
        factory->Release();

    return result;
}
