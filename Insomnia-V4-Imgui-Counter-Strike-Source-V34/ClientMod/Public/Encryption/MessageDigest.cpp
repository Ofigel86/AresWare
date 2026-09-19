//MessageDigest.cpp   
   
#include "MessageDigest.h"   
#include "DoubleBuffering.h"   
   
#include <stdexcept>
#include <exception>   // r47: fstream/strstream removed (mingw min/max macro clash)
   
using namespace std;   
   
//Digesting a Full File   
void IMessageDigest::DigestFile(string const& rostrFileIn, char* pcDigest)   
{   
    //Is the User's responsability to ensure that pcDigest is appropriately allocated   
    //Open Input File   
    FILE* in = fopen( rostrFileIn.c_str(), "rb" ); // r47: was ifstream
    if(!in)   
    {   
        throw runtime_error( "FileDigest ERROR: in IMessageDigest::DigestFile(): Cannot open File " + rostrFileIn + "!" );   
    }   
    //Resetting first   
    Reset();   
    //Reading from file   
    char szLargeBuff[BUFF_LEN+1] = {0};   
    char szBuff[DATA_LEN+1] = {0};   
    CDoubleBuffering oDoubleBuffering(in, szLargeBuff, BUFF_LEN, DATA_LEN);   
    int iRead;   
    while((iRead=oDoubleBuffering.GetData(szBuff)) > 0)   
        AddData(szBuff, iRead);   
    if( in ) fclose( in );   // r47
    //Final Step   
    FinalDigest(pcDigest);   
}   