#include "RecordManager.h"
using namespace std;
int main() {
	MyBitMap::initConst();
	FileManager* fileManager = new FileManager();
	BufPageManager* bufPageManager = new BufPageManager(fileManager);
	RecordManager* recordManager = new RecordManager(bufPageManager);
	
	int colSize[4] = { 1,2,3,4 };
	int colNum = 4;
	recordManager->createFile("test.db", colSize, colNum);
	int fileID;
	recordManager->openFile("test.db", fileID);
	cout << "fileID = " << fileID << endl;
	
	char record[11] = "aaaaaaaaaa";	
	int recordSize = sizeof(record) + sizeof(int);
	cout << "insertRecord = " << record << endl;
	int recordID;
	recordManager->insertRecord(fileID, recordSize, record, recordID);
	char record2[11];
	recordManager->queryRecord(fileID, recordID, 15, record2);
	cout << "queryRecord = " << record2 << endl;
	
	char record3[11] = "bbbbbbbbbb";
	recordManager->updateRecord(fileID, recordID, recordSize, record3);
	char record4[11];
	recordManager->queryRecord(fileID, recordID, 15, record4);
	cout << "queryRecord = " << record4 << endl;
	
	recordManager->deleteRecord(fileID, recordID, recordSize);
	if(!recordManager->queryRecord(fileID, recordID, 15, record2))
		cout << "delete record success" << endl;
	
	
	recordManager->closeFile(fileID);
	if(recordManager->deleteFile("test.db"))
		cout << "delete file success" << endl;
	return 0;
}
