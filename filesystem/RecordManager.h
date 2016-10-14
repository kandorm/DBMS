#include "bufmanager/BufPageManager.h"
#include "fileio/FileManager.h"
#include "utils/pagedef.h"

#define PAGE_AREA_RANGE 4096
#define PAGE_AREA_RANGE_HEAD 4097
//ÿ��4096ҳ����һFreePage��FreePage��ÿ2��byte����ĳҳ���пռ�
//�ʿ��Խ���4097ҳ����Ϊһ��ҳ��
class RecordManager {
private:
	BufPageManager* bufPageManager;
	//�����ļ�����ļ����г�ʼ����ҳͷ+��һ��FreePage��
	bool initFile(const char* name, int[] colSize, int colNum) {
		//��¼�ĳ���
		int recordSize = 0;
		for (int i = 0; i < colNum; i++)
			recordSize += colSize[i];
		//ÿһҳ��¼�����Ŀ
		int recordNumPerPage = PAGE_SIZE / recordSize;
		//��ֹ��ҳ�洢
		if (recordSize > PAGE_SIZE)return false;
		//��ʼ��ҳͷ�������ļ��м�¼����Ϣ��
		int fileID;
		int fileHeadIndex;
		bufPageManager->fileManager->openFile(name, fileID);
		BufType b = bufPageManager->allocPage(fileID, 0, fileHeadIndex, false);	//SIZE=PAGE_INT_NUM=2048
		b[0] = recordSize;												//�ļ��м�¼����
		b[1] = recordNumPerPage;									//�ļ�ÿһҳ���Դ洢����¼��
		b[2] = 0;														//�ļ��м�¼�ܸ���
		b[3] = 1;														//�ļ���ҳ���ĸ���
		b[4] = colNum;													//�������
		for (int i = 5; i < colNum + 5; i++)							//��ÿһ�е�size
			b[i] = colSize[i - 4];
		bufPageManager->markDirty(index);
		//�Խ���ͷ�ļ����FreePage���г�ʼ��
		if (initFreePage(fileID, 1, recordNumPerPage)) {
			bufPageManager->writeBack(index);
			bufPageManager->fileManager->closeFile(fileID);
		}
	}
	bool initFreePage(int fileID, int pageID, int recordNumPerPage) {
		int index;
		BufType b = bufPageManager->allocPage(fileID, pageID, index, false);
		for (int i = 0; i < PAGE_INT_NUM; i++)
			b[0] = recordNumPerPage<<16 + recordNumPerPage;
		bufPageManager->markDirty(index);
		return true;
	}
	bool _insertRecord(int fileID, int pageID, unsigned char* record) {
		int index;
		BufType b = bufPageManager->getPage(fileID, pageID, index);
	}
	int 
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
	bool insertRecord(int fileID, unsigned char* record) {
		int headIndex;
		BufType b = bufPageManager->getPage(fileID, 0, headIndex);
		int recordSize = b[0];			//���ļ��м�¼�ĳ��ȣ���¼�������UNSIGNED_MAX_INT��
		int recordNumPerPage = b[1];	//�ļ�ÿһҳ���Դ洢����¼��
		int pageAreaNum = b[3];			//�ļ���ҳ������
		
		for (int i = 0; i < pageAreaNum; i++) {
			int pageID = 1 + pageAreaNum * PAGE_AREA_RANGE_HEAD;
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
	bool deleteRecord(int fileID, int pageID, int recordID) {
		int fileHeadIndex;
		int recordSize;
		BufType fileHeadBuf = bufPageManager->getPage(fileID, 0, fileHeadIndex);
		recordSize = fileHeadBuf[0];
		
		BufType buf = bufPageManager->getPage(fileID, pageID, index);

	}
	bool updateRecord() {

	}
	int** getRecord(int fileID, unsigned char* record, int[] condCol) {

	}
	RecordManager() {
		MyBitMap::initConst();
		FileManager* fm = new FileManager();
		bufPageManager = new BufPageManager(fm);
	}
}
