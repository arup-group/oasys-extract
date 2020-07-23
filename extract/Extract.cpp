// Extract.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "ExtractData.h"
#include <filesystem>
#include <iostream>



struct CommandLineOptions
{
    std::string listPath;
    std::string htmlPath;
    bool brief;

    CommandLineOptions()
        : brief(false)
    {}
};

static void ShowUsage(std::string name)
{
    std::cerr << "Usage: " << name << " <option> input output"
        << "Options:\n"
        << "\t-h,--help\tShow this help message\n"
        << "\t-b,--brief\tSpecify brief output"
        << std::endl;
};

static int ProcessCommandLine(int argc, char* argv[], CommandLineOptions& options)
{
    bool list = false;
    bool html = false;
    options.brief = false;

    if (argc < 3)
    {
        ShowUsage(argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if ((arg == "-h") || (arg == "--help"))
        {
            ShowUsage(argv[0]);
            return 0;
        }
        else if (argv[i] == "-b" || argv[i] == "--brief")
        {
            options.brief = true;
        }
        else if (argv[i][0] == '-')
        {
            return 1;
        }
        else if (!list)
        {
            options.listPath = argv[i];
            list = true;
        }
        else if (!html)
        {
            options.htmlPath = argv[i];
            html = true;
        }
    }
    return 0;
};

int main(int argc, char* argv[])
{
    CommandLineOptions options;
    ProcessCommandLine(argc, argv, options);

    std::cout << argv[0] << '\n';

	//std::string listPath = "C:\\dev\\oasys-combined\\gsa\\gwdat\\gsa_textfile.lst";
	//std::string htmlPath = "C:\\dev\\oasys-combined\\gsa\\gwdat\\gsa_htmlfile.html";

    Oasys::CExtractData extractData;
	extractData.SetListPath(options.listPath);
	extractData.SetHtmlPath(options.htmlPath);
    extractData.SetBrief(options.brief);

	bool status = extractData.Process(); 

    if (status)
        std::cout << "Extract complete\n";
    else
        std::cout << "Extract terminated with errors\n";
 
	exit(status ? EXIT_SUCCESS : EXIT_FAILURE);
}
