#include "filesystem/bufmanager/BufPageManager.h"
#include "filesystem/fileio/FileManager.h"
#include "filesystem/utils/pagedef.h"
#include <string.h>

#define PAGE_AREA_RANGE 4096
#define PAGE_AREA_RANGE_HEAD 4097
#define PAGE_BEFORE_RECORD 1

//ÿ��4096ҳ����һManagerPage��ManagerPage��ÿ2��byte����ĳҳ���пռ�
//�ʿ��Խ���4097ҳ����Ϊһ��ҳ��,���ν����������
class RecordManager {
private:
	BufPageManager* bufPageManager;
	//�����ļ�����ļ����г�ʼ����ҳͷ+��һ��FreePage��
	bool initFile(const char* name, int colSize[], int colNum) {
		
		//��¼�ĳ���(��־λ+��¼)
		int recordSize = 0;
		for (int i = 0; i < colNum; i++)
			recordSize += colSize[i];
		recordSize += sizeof(int)+1;

		//��ֹ��ҳ�洢
		if (recordSize > PAGE_SIZE)return false;

		//��ʼ��ҳͷ�������ļ��м�¼����Ϣ��
		int fileID;
		int fileHeadIndex;
		bufPageManager->fileManager->openFile(name, fileID);
		BufType b = bufPageManager->allocPage(fileID, 0, fileHeadIndex, false);	//SIZE=PAGE_INT_NUM=2048
		b[0] = recordSize;												//�ļ��м�¼����
		b[1] = -1;														//�ļ������RecordID
		b[2] = 1;														//�ļ���ҳ���ĸ���
		b[3] = colNum;													//�������
		for (int i = 4; i < colNum + 4; i++)							//��ÿһ�е�size
			b[i] = colSize[i - 4];
		bufPageManager->markDirty(fileHeadIndex);
		//�Խ���ͷ�ļ����FreePage���г�ʼ��
		int recordNumPerPage = PAGE_SIZE / recordSize;
		if (initManagerPage(fileID, 1, recordNumPerPage)) {
			bufPageManager->writeBack(fileHeadIndex);
			bufPageManager->fileManager->closeFile(fileID);
		}
		return true;
	}
	bool initManagerPage(int fileID, int pageID, int recordNumPerPage) {
		int index;
		BufType b = bufPageManager->allocPage(fileID, pageID, index, false);
		for (int i = 0; i < PAGE_INT_NUM; i++)
			b[i] = recordNumPerPage<<16 | recordNumPerPage;
		
		bufPageManager->markDirty(index);
		bufPageManager->writeBack(index);
		return true;
	}
	bool _insertRecord(int fileID, int pageID, int recordSize, const char* record, int recordFreeNum, int& recordID) {
		int index;
		BufType b = bufPageManager->getPage(fileID, pageID, index);

		int recordPerPage = PAGE_SIZE / recordSize;
		int recordNum = recordPerPage - recordFreeNum;
		int baseRecordID = _pageID2BaseRecordID(pageID,recordSize);

		for (int i = 0; i < recordNum; i++) {
			b = b + i * recordSize;
			
			if (b[0] == -1) {
				recordID = baseRecordID;
				memcpy(b, &baseRecordID, sizeof(int));
				memcpy(b+sizeof(int), record, (recordSize-sizeof(int)));
			}
			baseRecordID++;
		}

		recordID = baseRecordID;
		if (recordNum == 0) {
			memcpy(b, &baseRecordID, sizeof(int));
			memcpy(b + sizeof(int), record, (recordSize - sizeof(int)));
		}
		else {
			b += recordSize;
			memcpy(b, &baseRecordID, sizeof(int));
			memcpy(b + sizeof(int), record, (recordSize - sizeof(int)));
		}
		int headIndex;
		BufType headBuf = bufPageManager->getPage(fileID, 0, headIndex);
		headBuf[1] = headBuf[1] < recordID ? recordID : headBuf[1];
		bufPageManager->markDirty(headIndex);
		bufPageManager->markDirty(index);
		return true;
	}
	int _recordIDtoPageID(int recordID, int recordSize) {
		int recordPerPage = PAGE_SIZE / recordSize;
		int onlyRecordPageIndex = recordID / recordPerPage;
		int managerPageNum = onlyRecordPageIndex / PAGE_AREA_RANGE + 1;

		return PAGE_BEFORE_RECORD + onlyRecordPageIndex + managerPageNum;
	}
	
	int _pageID2BaseRecordID(int pageID, int recordSize)   	{
		int pageWithoutRecord = (pageID-1) / PAGE_AREA_RANGE_HEAD + 1 + PAGE_BEFORE_RECORD;
		int pageWithRecord = pageID - pageWithoutRecord;
		int recordPerPage = PAGE_SIZE / recordSize;
		
		int baseRecordID = pageWithRecord * recordPerPage;
		return baseRecordID;
	}
	
public:
	//File Operation
	bool createFile(const char* name, int colSize[], int colNum) {
		remove(name);
		if (bufPageManager->fileManager->createFile(name)) {
			if (initFile(name, colSize, colNum))
				return true;
			remove(name);
			return false;
		}
		return false;
	}
	bool deleteFile(const char* name) {
		if (remove(name) == 0)
			return true;
		return false;
	}
	bool openFile(const char* name, int& fileID) {
		return bufPageManager->fileManager->openFile(name, fileID);
	}
	int closeFile(int fileID) {
		bufPageManager->close();
		bufPageManager->fileManager->closeFile(fileID);
		return 0;
	}

	//Record Operation
	bool insertRecord(int fileID, int recordSize, const char* record, int& recordID) {
		int headIndex;
		BufType b = bufPageManager->getPage(fileID, 0, headIndex);
		
		int recordPerPage = PAGE_SIZE / recordSize;
		int pageAreaNum = b[2];			//�ļ���ҳ������
		
		for (int i = 0; i < pageAreaNum; i++) {
			int pageID = 1 + i * PAGE_AREA_RANGE_HEAD;
			int freePageIndex;
			BufType bFreePage = bufPageManager->getPage(fileID, pageID, freePageIndex);
			
			for (int j = 0; j < PAGE_INT_NUM; j++) {
				if (bFreePage[j] != 0) {
					int recordPageID;
					int recordFreeNum;
					if (bFreePage[j] >> 16 != 0) {
						recordPageID = pageID + 2 * (j+1) - 1;
						recordFreeNum = bFreePage[j] >> 16;	
					}
					else {
						recordPageID = pageID + 2 * (j + 1);
						recordFreeNum = bFreePage[j];	
					}
					if (_insertRecord(fileID, recordPageID, recordSize, record, recordFreeNum, recordID)) {
						//�ļ�ͷ��¼����1
						b[2] += 1;
						bufPageManager->markDirty(headIndex);
						//��Ӽ�¼ҳ��Ӧ��FreePage�и�ҳ���пռ��1
						if (bFreePage[j] >> 16 != 0)
							bFreePage[j] -= 1<<16;
						else
							bFreePage[j] -= 1;
						bufPageManager->markDirty(freePageIndex);
						return true;
					}
					return false;
				}
			}
		}
		//�ִ�ҳ������
		int newFreePageID = 1 + pageAreaNum * PAGE_AREA_RANGE_HEAD;
		initManagerPage(fileID, newFreePageID, recordPerPage);
		if (_insertRecord(fileID, newFreePageID + 1, recordSize, record, recordPerPage, recordID)) {
			//�ļ�ͷ��¼����1
			b[2] += 1;
			bufPageManager->markDirty(headIndex);
		}
		return true;


	}
	bool deleteRecord(int fileID, int recordID, int recordSize) {
		
		int recordPageID = _recordIDtoPageID(recordID, recordSize);
		int recordPageIndex;
		BufType recordPageBuf = bufPageManager->getPage(fileID, recordPageID, recordPageIndex);
		
		int recordPerPage = PAGE_SIZE / recordSize;
		int recordIndex = recordID % recordPerPage;
		int newRecordID = -1;
		memcpy(recordPageBuf+recordIndex*recordSize, &newRecordID, sizeof(int));//change
		bufPageManager->markDirty(recordPageIndex);
		return true;


	}
	bool updateRecord(int fileID, int recordID, int recordSize, const char* record) {
		
		int recordPageID = _recordIDtoPageID(recordID, recordSize);
		int recordPageIndex;
		BufType recordPageBuf = bufPageManager->getPage(fileID, recordPageID, recordPageIndex);

		int recordPerPage = PAGE_SIZE / recordSize;
		int recordIndex = recordID % recordPerPage;
		memcpy(recordPageBuf+recordIndex*recordSize+sizeof(int), record, recordSize-sizeof(int));
		bufPageManager->markDirty(recordPageIndex);
		return true;

	}
	bool queryRecord(int fileID, int recordID, int recordSize, char* record) {

		int headPageIndex;
		BufType headPageBuf = bufPageManager->getPage(fileID, 0, headPageIndex);
		if (recordID > headPageBuf[1])return false;

		int recordPageID = _recordIDtoPageID(recordID, recordSize);
		int recordPageIndex;
		BufType recordPageBuf = bufPageManager->getPage(fileID, recordPageID, recordPageIndex);

		int recordPerPage = PAGE_SIZE / recordSize;
		int recordIndex = recordID % recordPerPage;
		int flag;
		memcpy(&flag, recordPageBuf+recordIndex*recordSize, sizeof(int));
		if (flag == -1)return false;
		memcpy(record, recordPageBuf+recordIndex*recordSize+sizeof(int), recordSize-sizeof(int));
		return true;
	}
	RecordManager(BufPageManager* bpm) {
		bufPageManager = bpm;
	}
};
