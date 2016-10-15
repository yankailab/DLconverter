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

int main(int argc, const char ** argv)
{
	if (argc <= 2)
	{
		printf("usage: dlconv [input folder name] [output folder name]\n");
		exit(1);
	}

	string dirIn = argv[1];
	string dirOut = argv[2];

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

	int nFile = 0;
	int nSuccess = 0;

	while ((dir = readdir(pDir)) != NULL)
	{
		string fileName = dir->d_name;
		fileName = dirIn + fileName;

		size_t extpos = fileName.find(".xml");
		if (extpos == std::string::npos)continue;
		if (fileName.substr(extpos) != ".xml")continue;

		printf("[%d]\t%s\t: ", nFile++, fileName.c_str());

		XMLDocument* pXML = new XMLDocument();
		pXML->LoadFile(fileName.c_str());
		int errorID = pXML->ErrorID();

		if(errorID!=0)
		{
			printf("Error: %d\n", errorID);
			pXML->PrintError();
			printf("\n");
			delete pXML;
			continue;
		}

		printf("Parsed: ");

		XMLElement* pAnnotation = pXML->FirstChildElement("annotation");
		if(!pAnnotation)
		{
			printf("<annotation> not found\n");
			delete pXML;
			continue;
		}

		XMLElement* pFilename = pAnnotation->FirstChildElement("filename");
		if(!pFilename)
		{
			printf("<filename> not found\n");
			delete pXML;
			continue;
		}
		string outFileName = dirOut + pFilename->GetText();
		outFileName	+= ".txt";

		XMLElement* pObject = pAnnotation->FirstChildElement("object");
		if(!pObject)
		{
			printf("<object> not found\n");
			delete pXML;
			continue;
		}

		XMLElement* pName = pObject->FirstChildElement("name");
		if(!pName)
		{
			printf("<name> not found\n");
			delete pXML;
			continue;
		}
		string objName = pName->GetText();

		XMLElement* pBndbox = pObject->FirstChildElement("bndbox");
		if(!pBndbox)
		{
			printf("<bndbox> not found\n");
			delete pXML;
			continue;
		}


		XMLElement* pXmin = pBndbox->FirstChildElement("xmin");
		if(!pXmin)
		{
			printf("<xmin> not found\n");
			delete pXML;
			continue;
		}

		XMLElement* pYmin = pBndbox->FirstChildElement("ymin");
		if(!pYmin)
		{
			printf("<ymin> not found\n");
			delete pXML;
			continue;
		}

		XMLElement* pXmax = pBndbox->FirstChildElement("xmax");
		if(!pXmax)
		{
			printf("<xmax> not found\n");
			delete pXML;
			continue;
		}

		XMLElement* pYmax = pBndbox->FirstChildElement("ymax");
		if(!pYmax)
		{
			printf("<ymax> not found\n");
			delete pXML;
			continue;
		}

		string xmin = pXmin->GetText();
		string ymin = pYmin->GetText();
		string xmax = pXmax->GetText();
		string ymax = pYmax->GetText();

		delete pXML;
//		printf("%s,[%s,%s,%s,%s] ",objName.c_str(),xmin.c_str(),ymin.c_str(),xmax.c_str(),ymax.c_str());

		//Output
		string kitti = "";

		//KITTI data format per line
		//Type, Truncated, Occluded, Angle, BBox[4], Dim[3], Loc[3], Rot
		kitti += objName + " 0 0 0 " + xmin + " " + ymin + " " + xmax + " " + ymax + " 0 0 0 0 0 0 0";
		printf("[%s] ", kitti.c_str());

		ofstream kittiFile;
		kittiFile.open(outFileName.c_str(),ios::out);
	    if(!kittiFile)
		{
			printf("Cannot open output file\n");
	    	continue;
	    }

	    kittiFile.write(kitti.c_str(), kitti.length());
	    kittiFile.close();

		printf("\tConverted\n");
		nSuccess++;

	}

	closedir(pDir);

	printf("%d files processed\n", nFile);
	printf("%d files converted\n", nSuccess);

	return 0;

}

/*
 <?xml version="1.0" ?>
 <annotation>
 <folder>HondaChargeStation</folder>
 <filename>P1010105</filename>
 <path>/home/kang/Downloads/HondaChargeStation/P1010105.JPG</path>
 <source>
 <database>Unknown</database>
 </source>
 <size>
 <width>2048</width>
 <height>1536</height>
 <depth>3</depth>
 </size>
 <segmented>0</segmented>
 <object>
 <name>miimo_charge_station</name>
 <pose>Unspecified</pose>
 <truncated>0</truncated>
 <difficult>0</difficult>
 <bndbox>
 <xmin>777</xmin>
 <ymin>627</ymin>
 <xmax>1309</xmax>
 <ymax>796</ymax>
 </bndbox>
 </object>
 </annotation>
 */

/*
 Pedestrian 0.00 0 -0.20 712.40 143.00 810.73 307.92 1.89 0.48 1.20 1.84 1.47 8.41 0.01
 */
