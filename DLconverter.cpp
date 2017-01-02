#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <fstream>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include <string.h>
#include <iostream>

#include "include/tinyxml2.h"

using namespace std;
using namespace tinyxml2;

bool addObj(XMLElement* pObj, string* pOut, string* pErr);

int main(int argc, const char ** argv)
{
	if (argc <= 3)
	{
		printf("usage: ./dlconv [input folder name] [output folder name] [image folder]\n");
		exit(1);
	}

	string dirIn = argv[1];
	string dirOut = argv[2];
	string dirImg = argv[3];

	dirIn += "/";
	dirOut += "/";

	DIR* pDir;
	struct dirent *dir;
	pDir = opendir(dirIn.c_str());

	if (!pDir)
	{
		printf("Directory not found");
		exit(0);
	}

	string errAll = "";
	int nFile = 0;
	int nSuccess = 0;

	while ((dir = readdir(pDir)) != NULL)
	{
		string fileName = dir->d_name;
		fileName = dirIn + fileName;

		size_t extpos = fileName.find(".xml");
		if (extpos == std::string::npos)continue;
		if (fileName.substr(extpos) != ".xml")continue;

		printf("[%d]\t%s\n", nFile++, fileName.c_str());

		XMLDocument* pXML = new XMLDocument();
		pXML->LoadFile(fileName.c_str());
		int errorID = pXML->ErrorID();

		if(errorID!=0)
		{
			errAll += fileName + " : "+ pXML->ErrorName() + "\n";
			delete pXML;
			continue;
		}

		XMLElement* pAnnotation = pXML->FirstChildElement("annotation");
		if(!pAnnotation)
		{
			errAll += fileName + " : <annotation> not found\n";
			delete pXML;
			continue;
		}

		XMLElement* pFilename = pAnnotation->FirstChildElement("filename");
		if(!pFilename)
		{
			errAll += fileName + " : <filename> not found\n";
			delete pXML;
			continue;
		}


		string err = "";
		string kitti = "";

		XMLElement* pObject = pAnnotation->FirstChildElement("object");
		while(pObject)
		{
			if(!addObj(pObject, &kitti, &err))
			{
				errAll += fileName + " : " + err;
				continue;
			}

			pObject = pObject->NextSiblingElement("object");
		}

		if(kitti.length()<=0)
		{
			errAll += fileName + " : no object found\n";
			delete pXML;
			continue;
		}

		//is it needed to erase the last \n?
		kitti.erase(kitti.length()-1);

		string outFileName = dirOut + pFilename->GetText();
		size_t pDot = outFileName.find_last_of(".");
		if (pDot != std::string::npos)
		{
			outFileName.erase(pDot);
		}
		outFileName	+= ".txt";


		ofstream kittiFile;
		kittiFile.open(outFileName.c_str(),ios::out);
	    if(!kittiFile)
		{
			errAll += fileName + " : " + outFileName + " : open failed\n";
	    	continue;
	    }

	    kittiFile.write(kitti.c_str(), kitti.length());
	    kittiFile.close();

		nSuccess++;
		delete pXML;
	}

	closedir(pDir);

	printf("-----------------------------------------------------\n");
	printf("%s\n",errAll.c_str());
	printf("%d files processed\n", nFile);
	printf("%d files converted\n", nSuccess);

	return 0;

}

bool addObj(XMLElement* pObj, string* pOut, string* pErr)
{
	if(pObj==NULL)return false;
	if(pOut==NULL)return false;
	if(pErr==NULL)return false;

	XMLElement* pName = pObj->FirstChildElement("name");
	if(!pName)
	{
		*pErr = "<name> not found\n";
		return false;
	}
	string objName = pName->GetText();

	XMLElement* pBndbox = pObj->FirstChildElement("bndbox");
	if(!pBndbox)
	{
		*pErr = "<bndbox> not found";
		return false;
	}

	XMLElement* pXmin = pBndbox->FirstChildElement("xmin");
	if(!pXmin)
	{
		*pErr = "<xmin> not found\n";
		return false;
	}

	XMLElement* pYmin = pBndbox->FirstChildElement("ymin");
	if(!pYmin)
	{
		*pErr = "<ymin> not found\n";
		return false;
	}

	XMLElement* pXmax = pBndbox->FirstChildElement("xmax");
	if(!pXmax)
	{
		*pErr = "<xmax> not found\n";
		return false;
	}

	XMLElement* pYmax = pBndbox->FirstChildElement("ymax");
	if(!pYmax)
	{
		*pErr = "<ymax> not found\n";
		return false;
	}

	string xmin = pXmin->GetText();
	string ymin = pYmin->GetText();
	string xmax = pXmax->GetText();
	string ymax = pYmax->GetText();

	//KITTI data format per line
	//Type, Truncated, Occluded, Angle, BBox[4], Dim[3], Loc[3], Rot
	*pOut += objName + " 0 0 0 " + xmin + " " + ymin + " " + xmax + " " + ymax + " 0 0 0 0 0 0 0\n";

	return true;
}
