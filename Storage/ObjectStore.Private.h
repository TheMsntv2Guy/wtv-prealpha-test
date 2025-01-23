#ifndef __OBJECTSTORE_PRIVATE_H__
#define __OBJECTSTORE_PRIVATE_H__

static FSNode 		*ResolveAtNode(FSNode *node, const char *name, Boolean acceptParent);
static Filesystem 	*GetFilesystemByName(uchar *name);


#endif /* __OBJECTSTORE_PRIVATE_H__ */
