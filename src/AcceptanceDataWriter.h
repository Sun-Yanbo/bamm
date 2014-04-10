#ifndef ACCEPTANCE_DATA_WRITER_H
#define ACCEPTANCE_DATA_WRITER_H


#include <string>
#include <fstream>

class Settings;
class Model;
class MCMC;


class AcceptanceDataWriter
{
public:

    AcceptanceDataWriter(Settings& settings);
    ~AcceptanceDataWriter();

    void writeData(MCMC& mcmc);

private:

    void initializeStream();
    void writeHeader();
    std::string header();

    std::string _outputFileName;
    std::ofstream _outputStream;
};


#endif
