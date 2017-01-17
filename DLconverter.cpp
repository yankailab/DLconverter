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
#include <sys/types.h>
#include <sys/stat.h>

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

	//Get directories
	string dirIn = argv[1];
	string dirOut = argv[2];
	string dirImg = argv[3];

	if(dirIn.at(dirIn.length()-1)!='/')dirIn.push_back('/');
	if(dirOut.at(dirOut.length()-1)!='/')dirOut.push_back('/');
	if(dirImg.at(dirImg.length()-1)!='/')dirImg.push_back('/');

	DIR* pDirIn;
	DIR* pDirOut;
	DIR* pDirImg;

	pDirIn = opendir(dirIn.c_str());
	pDirImg = opendir(dirImg.c_str());
	pDirOut = opendir(dirOut.c_str());

	if (!pDirIn)
	{
		cout << "Input directory not found" << endl;
		exit(0);
	}

	if (!pDirImg)
	{
		cout << "Image directory not found" << endl;
		exit(0);
	}

	if (!pDirOut)
	{
		if(mkdir(dirOut.c_str(), 0777)!=0)
		{
			cout << "mkdir failed: " << dirOut << endl;
			exit(0);
		}

		pDirOut = opendir(dirOut.c_str());
		if(!pDirOut)
		{
			cout << "Directory opening failed: " << dirOut << endl;
			exit(0);
		}
	}

	struct dirent *dir;
	string fileIn;
	string fileImg;
	size_t extPos;
	ifstream inFile;

	//Delete non xml files
	while ((dir = readdir(pDirIn)) != NULL)
	{
		fileIn = dirIn + dir->d_name;

		extPos = fileIn.find(".xml");
		if (extPos == std::string::npos)
		{
			remove(fileIn.c_str());
			continue;
		}

		if (fileIn.substr(extPos) != ".xml")
		{
			remove(fileIn.c_str());
			continue;
		}

		//delete if no correspondent image file is found
		fileImg = dirImg + dir->d_name;
		fileImg.erase(fileImg.find(".xml"));
		fileImg += ".jpg";
		inFile.open(fileImg.c_str(),ios::in);
	    if(!inFile)
		{
			remove(fileIn.c_str());
	    	continue;
	    }
	    else
	    {
	    	inFile.close();
	    }
	}

	//Delete non jpg files
	while ((dir = readdir(pDirImg)) != NULL)
	{
		fileImg = dirImg + dir->d_name;

		extPos = fileImg.find(".jpg");
		if (extPos == std::string::npos)
		{
			remove(fileImg.c_str());
			continue;
		}

		if (fileImg.substr(extPos) != ".jpg")
		{
			remove(fileImg.c_str());
			continue;
		}

		//delete if no correspondent xml file is found
		fileIn = dirIn+ dir->d_name;
		fileIn.erase(fileIn.find(".jpg"));
		fileIn += ".xml";
		inFile.open(fileIn.c_str(),ios::in);
	    if(!inFile)
		{
			remove(fileImg.c_str());
	    	continue;
	    }
	    else
	    {
	    	inFile.close();
	    }
	}

	//Convert PASCAL VOC into kitti
	string errAll = "";
	int nFile = 0;
	int nSuccess = 0;

	closedir(pDirIn);
	pDirIn = opendir(dirIn.c_str());

	while ((dir = readdir(pDirIn)) != NULL)
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

	closedir(pDirIn);

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


