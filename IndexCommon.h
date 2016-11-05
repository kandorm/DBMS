#ifndef INDEX_COMMON
#define INDEX_COMMON

enum IndexType {
	INT,
	CHAR
};

enum PageType {
	NODE,
	LEAF
}

enum Status {
	NOTHING,
	SPLIT,
	MERGE
};

typedef struct IndexFileHead {
	IndexType indexType;
	int indexSize;
	int firstLeafNode;
	int root;
	int pageNum;
	int firstFreePage;
};

typedef struct PageHead {
	PageType pageType;
	int n;
}

#endif // !INDEX_COMMON
