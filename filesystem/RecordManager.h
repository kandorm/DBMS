#include "bufmanager/BufPageManager.h"
#include "fileio/FileManager.h"
#include "utils/pagedef.h"

#define PAGE_AREA_RANGE 4096
#define PAGE_AREA_RANGE_HEAD 4097
#define PAGE_BEFORE_RECORD 1

//ÿ��4096ҳ����һManagerPage��ManagerPage��ÿ2��byte����ĳҳ���пռ�
//�ʿ��Խ���4097ҳ����Ϊһ��ҳ��,���ν����������
class RecordManager {
private:
	BufPageManager* bufPageManager;
	//�����ļ�����ļ����г�ʼ����ҳͷ+��һ��FreePage��
	bool initFile(const char* name, int[] colSize, int colNum) {
		
		//��¼�ĳ���(��־λ+��¼)
		int recordSize = 0;
		for (int i = 0; i < colNum; i++)
			recordSize += colSize[i];
		recordSize += sizeof(int);

		//��ֹ��ҳ�洢
		if (recordSize > PAGE_SIZE)return false;

		//��ʼ��ҳͷ�������ļ��м�¼����Ϣ��
		int fileID;
		int fileHeadIndex;
		bufPageManager->fileManager->openFile(name, fileID);
		BufType b = bufPageManager->allocPage(fileID, 0, fileHeadIndex, false);	//SIZE=PAGE_INT_NUM=2048
		b[0] = recordSize;												//�ļ��м�¼����
		b[1] = 0;														//�ļ��м�¼�ܸ���
		b[2] = 1;														//�ļ���ҳ���ĸ���
		b[3] = colNum;													//�������
		for (int i = 4; i < colNum + 4; i++)							//��ÿһ�е�size
			b[i] = colSize[i - 4];
		bufPageManager->markDirty(index);
		//�Խ���ͷ�ļ����FreePage���г�ʼ��
		if (initFreePageManagerPage(fileID, 1, recordNumPerPage)) {
			bufPageManager->writeBack(index);
			bufPageManager->fileManager->closeFile(fileID);
		}
	}
	bool initFreePageManagerPage(int fileID, int pageID, int recordNumPerPage) {
		int index;
		BufType b = bufPageManager->allocPage(fileID, pageID, index, false);
		for (int i = 0; i < PAGE_INT_NUM; i++)
			b[i] = recordNumPerPage<<16 + recordNumPerPage;
		bufPageManager->markDirty(index);
		return true;
	}
	bool _insertRecord(int fileID, int pageID, unsigned char* record) {
		int index;
		BufType b = bufPageManager->getPage(fileID, pageID, index);
	}
	int _recordIDtoPageID(int recordID, int recordSize) {
		int recordPerPage = PAGE_SIZE / recordSize;
		int onlyRecordPageIndex = recordID / recordPerPage;
		int managerPageNum = onlyRecordPageIndex / PAGE_AREA_RANGE + 1;

		return PAGE_BEFORE_RECORD + onlyRecordPageIndex + managerPageNum;
	}
public:
	//File Operation
	bool createFile(const char* name, int[] colSize, int colNum) {
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
		return bufPageManager->fileManager->closeFile(fileID);
	}

	//Record Operation
	bool insertRecord(int fileID, int recordSize, unsigned char* record) {
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
					if (bFreePage[j] >> 16 != 0)
						recordPageID = pageID + 2 * (j+1) - 1;
					else 
						recordPageID = pageID + 2 * (j + 1);
					if (_insertRecord(fileID, recordPageID, record)) {
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
		initFreePage(fileID, newFreePageID, recordNumPerPage);
		if (_insertRecord(fileID, newFreePageID + 1, record)) {
			//�ļ�ͷ��¼����1
			b[2] += 1;
			bufPageManager->markDirty(headIndex);
		}


	}
	bool deleteRecord(int fileID, int recordID, int recordSize) {
		
		int recordPageID = _recordIDtoPageID(recordID, recordSize);
		int recordPageIndex;
		BufType recordPageBuf = bufPageManager->getPage(fileID, recordPageID, recordPageIndex);
		
		int recordPerPage = PAGE_SIZE / recordSize;
		int recordIndex = recordID % recordPerPage;
		int newRecordID = -1;
		memcpy(recordPageBuf[recordIndex*recordSize], &newRecordID, sizeof(int));
		bufPageManager->markDirty(recordPageIndex);
		return true;


	}
	bool updateRecord(int fileID, int recordID, int recordSize, unsigned char* record) {
		
		int recordPageID = _recordIDtoPageID(recordID, recordSize);
		int recordPageIndex;
		BufType recordPageBuf = bufPageManager->getPage(fileID, recordPageID, recordPageIndex);

		int recordPerPage = PAGE_SIZE / recordSize;
		int recordIndex = recordID % recordPerPage;
		memcpy(recordPageBuf[recordIndex*recordSize], record, recordSize);
		bufPageManager->markDirty(recordPageIndex);
		return true;

	}
	char* queryRecord(int fileID, int recordID, int recordSize) {

		int recordPageID = _recordIDtoPageID(recordID, recordSize);
		int recordPageIndex;
		BufType recordPageBuf = bufPageManager->getPage(fileID, recordPageID, recordPageIndex);

		int recordPerPage = PAGE_SIZE / recordSize;
		int recordIndex = recordID % recordPerPage;
		char* record;
		memcpy(record, recordPageBuf[recordIndex*recordSize], recordSize);
		return record;
	}
	RecordManager(BufPageManager bpm) {
		bufPageManager = bpm;
	}
}
