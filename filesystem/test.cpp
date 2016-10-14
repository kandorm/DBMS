#include "bufmanager/BufPageManager.h"
#include "fileio/FileManager.h"
#include "utils/pagedef.h"
#include <iostream>

using namespace std;

int main() {
	MyBitMap::initConst();   //新加的初始化
	FileManager* fm = new FileManager();
	BufPageManager* bpm = new BufPageManager(fm);
	fm->createFile("testfile.txt"); //新建文件
	int fileID;
	fm->openFile("testfile.txt", fileID); //打开文件，fileID是返回的文件id
	for (int pageID = 0; pageID < 1000; ++pageID) {
		int index;
		//为pageID获取一个缓存页
		BufType b = bpm->getPage(fileID, pageID, index, false);
		//注意，在allocPage或者getPage后，千万不要进行delete[] b这样的操作
		//内存的分配和管理都在BufPageManager中做好，不需要关心，如果自行释放会导致问题
		for (int i = 0; i < 8192; i++)
			cout << "b[" << i << "]=" <<b[i] << endl;
		b[0] = pageID; //对缓存页进行写操作
		b[1] = fileID;
		bpm->markDirty(index); //标记脏页
							   //在重新调用allocPage获取另一个页的数据时并没有将原先b指向的内存释放掉
							   //因为内存管理都在BufPageManager中做好了
	}
	bpm->close();
	return 0;
}