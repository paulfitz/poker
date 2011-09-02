/*--Include File Header--------------------------------------------------------

Module		mesh
                                                                           
Author		Paul Fitzpatrick, 4th Comp Eng, 9117261
Version		1.0
Date		15/11/94
Description This file declares classes to be used when setting up a Mesh
			data structure.

			A mesh is a loosely structured collections of meshlinks.
			Each meshlink has a source and a destination.
			The source and destination are other meshlinks.
			New meshlink classes can be derived to hold user data.

			The classes in this header implement meshes with the following
			properties
			-	For a meshlink L, all meshlinks that have L as their source
				can be found efficiently, without a search
			-	All meshlinks that have L as their destination can be found
				efficiently, without a search
			-	All meshlinks in a given mesh can be found efficiently
			-	Deleting or modifying a meshlink can be done safely - any
				meshlinks connected to the changing meshlink are updated
				automatically

			Implementing these properties requires a meshlink to be a node
			on three separate double-linked lists -
			-	A list of all meshlinks in a mesh
			-	A list of all meshlinks with a given meshlink as their
				source
			-	A list of all meshlinks with a given meshlink as their
				destination


			Using the classes:

			Suppose a homogeneous mesh is being set up - a mesh in which all
			nodes are of the same class. The declarations would be as
			follows:

			class UserMeshLink : public BasedLink<UserMeshLink>
			{
				//	New user-defined members
				.
				.
				.
			};


			class UserMesh : public MeshOf<UserMeshLink>
			{
				//	New user-defined members
				.
				.
				.
			};


			All UserMesh and UserMeshLink information functions will return
			pointers to UserMeshLinks, so typecasts in application code
			will seldom be necessary.
-----------------------------------------------------------------------------*/


#ifndef MESH_INC
#define MESH_INC


//--Includes-------------------------------------------------------------------
//-----------------------------------------------------------------------------

#include <iostream>
#ifndef __QNX4__
using namespace std;
#endif
#include <assert.h>

#include "YARPString.h"

//--Forward Declarations-------------------------------------------------------
//-----------------------------------------------------------------------------

//	Forward declarations
class MeshLink;
class Mesh;


//--Class Declarations---------------------------------------------------------
//-----------------------------------------------------------------------------

/*-----------------------------------------------------------------------------
Class		MeshLink
                                                                           
Description	A MeshLink object is a single node in a mesh.
-----------------------------------------------------------------------------*/
class MeshLink
{
protected:
  int pruneId;
	string
		label;			//	Name of node
	MeshLink
		*src,			//	Source of link
		*dest;			//	Destination of link		
	MeshLink
		*asSrcRoot,		//	Root of list of links with this link as
            			//	their source
		*asDestRoot;	//	Root of list of links with this link as
						//	their destination
	MeshLink
		*srcPrev,		//	Previous link with the same source
		*srcNext,		//	Next link with the same source
		*destPrev,		//	Previous link with the same destination
		*destNext;		//	Next link with the same destination
	MeshLink
		*meshPrev,		//	Previous link in the mesh
		*meshNext;		//	Next link in the mesh
	Mesh
		*owner;			//	Mesh in which this link is contained

public:
  int GetId()
  { return pruneId; }

  void SetId(int id)
  { pruneId = id; }

	//	Constructor
	MeshLink();

	//	Destructor
	virtual ~MeshLink();

	//true if anyone sources from this, or acts as a dest for it
	int HasOutput()
	{ return ( dest != NULL ) || ( asSrcRoot != NULL ); }

	int HasInput()
	{ return ( src != NULL ) || ( asDestRoot != NULL ); }

	//	Sets the source of this link, disconnecting from any previously
	//	connected source
	void SetSource ( MeshLink *link );

	//	Sets the destination of this link, disconnecting from any
	//	previously connected destination
	void SetDest ( MeshLink *link );

	//	Sets the mesh in which this link is contained, disconnecting
	//	from any previously connected mesh
	void SetOwner ( Mesh *mesh );

	//	Sets or changes the name of the link
	void SetLabel ( const string& lbl );
	void SetLabel ( const char *lbl );

	//	Disconnects any links using this link as their source
	void RemoveAsSource();

	//	Disconnects any links using this link as their destination
	void RemoveAsDest();

	//	Gets the name of the link
	const string& GetLabel()	{	return label;		}

	Mesh *GetOwner()	{ return owner; }

#ifndef ROBOT
	//	Reads the user-data component of the link in verbose format
	//	(For example, from cin)
	virtual int Read ( istream& is, int style )	{	return 1;	}

	//	Writes the user-data component of the link in verbose format
	//	(For example, to cout)
	virtual int Write ( ostream& os, int style )	{	return 1;	}
#endif

	//	Traversal functions

	//	Gets current source of link
	MeshLink *BaseGetSource()	{	return src;			}

	//	Gets current destination of link
	MeshLink *BaseGetDest()		{	return dest;		}

	//	Gets previous link in the mesh
	MeshLink *BaseGetMeshPrev()	{	return meshPrev;		}

	//	Gets next link in the mesh
	MeshLink *BaseGetMeshNext()	{	return meshNext;		}

	//	Gets root of list of all links that use this link as their
	//	source
	MeshLink *BaseGetAsSrcRoot()	{	return asSrcRoot;		}

	//	Gets previous link that uses this link as its source
	MeshLink *BaseGetAsSrcPrev()	{	return srcPrev;		}

	//	Gets next link that uses this link as its source
	MeshLink *BaseGetAsSrcNext()	{	return srcNext;		}

	//	Gets root of list of all links that use this link as their
	//	destination
	MeshLink *BaseGetAsDestRoot()	{	return asDestRoot;	}

	//	Gets previous link that uses this link as its destination
	MeshLink *BaseGetAsDestPrev()	{	return destPrev;		}

	//	Gets next link that uses this link as its destination
	MeshLink *BaseGetAsDestNext()	{	return destNext;		}

	void Mark(int id);
};



extern const string
	NullName;		//	Sentinel name to signal the end of a list of
					//	links		 

/*-----------------------------------------------------------------------------
Class		Mesh
                                                                           
Description	Manager for MeshLinks.
			Suitable for I/O and deletion.
			Targetted to solve problem of meshes not necessarily being fully
			connected.
			Each MeshLink can be given a tag for identification.
-----------------------------------------------------------------------------*/
class Mesh
{
	friend class MeshLink;

protected:
	MeshLink
		*root;			//	First link in Mesh

	int autoClear;

	//	Creates a link of the default type.
    //	This is overridden for Meshes of user-defined Links
	virtual MeshLink *CreateLink()	{	return new MeshLink;	}

	//	Makes sure that a link with the given label exists. If it doesn't,
    //	create it
	MeshLink *LabelledLink ( const string& label );

    //	Adds a new link from a given source to a given destination
	MeshLink *AddLink ( const string& label, const string& srcLabel, const string& destLabel );

public:

    //	Styles to control input and output of the mesh
	enum StyleConstants
	{
		CONCISE_FORMAT		= 0x0000,
		PROSE_FORMAT		= 0x0001,
		FORMAT				= 0x0001,
		MINIMAL_CONTENT		= 0x0000,
		EXHAUSTIVE_CONTENT	= 0x0002,
		CONTENT				= 0x0002,
		ARCHIVE_INPUT_STYLE	= CONCISE_FORMAT + EXHAUSTIVE_CONTENT,
		ARCHIVE_OUTPUT_STYLE= ARCHIVE_INPUT_STYLE,
		USER_INPUT_STYLE	= CONCISE_FORMAT + MINIMAL_CONTENT,
		USER_OUTPUT_STYLE	= PROSE_FORMAT + EXHAUSTIVE_CONTENT
	};

    //	Constructor
	Mesh();

    //	Destructor
	virtual ~Mesh();

    //	Finds the link in the mesh with the given label
	MeshLink *FindByLabel ( const string& label );

	//	Deletes all the links in the mesh
	void Clear();

        void Prune(MeshLink *link);

	void SetAutoClear ( int val) { autoClear = val; }

  int Size();

#ifndef ROBOT
	//	Inputs mesh from a stream using the given style
	int Load ( istream& is, int style = ARCHIVE_INPUT_STYLE );

	//	Outputs mesh to a stream using the given style
	int Store ( ostream& os, int style = ARCHIVE_OUTPUT_STYLE );

	//	Inputs mesh from a stream (Concise format, Minimal content)
	friend istream& operator >> ( istream& is, Mesh& mesh );

	//	Outputs mesh to a stream (Prose format, Exhaustive content)
	friend ostream& operator << ( ostream& os, Mesh& mesh );
#endif

};


//--Templated Class Declarations-----------------------------------------------
//-----------------------------------------------------------------------------

/*-----------------------------------------------------------------------------
Class		BasedLink
                                                                           
Description	BasedLink is a templated shell for MeshLink used when all the
			links in a mesh have a common base class other than MeshLink
			itself.
			In homogenous meshes, where all the links are of type UserLink,
			UserLink could be declared as:
				class UserLink : public BasedLink<UserLink>

Template	BaseType		The common ancestor of all the MeshLinks in the
							mesh.
-----------------------------------------------------------------------------*/

template <class BaseType>
class BasedLink : public MeshLink
{
public:
	//	Tracing functions

	//	Gets current source of link
	BaseType *GetSource()		{	return ( BaseType * ) src;		}
	//	Gets current destination of link
	BaseType *GetDest()		{	return ( BaseType * ) dest;		}

	//	Gets previous link in the mesh
	BaseType *GetMeshPrev()		{	return ( BaseType * ) meshPrev; 	}
	//	Gets next link in the mesh
	BaseType *GetMeshNext()		{	return ( BaseType * ) meshNext;	}

	//	Gets root of list of all links that use this link as their
      //	source
	BaseType *GetAsSrcRoot()	{	return ( BaseType * ) asSrcRoot;	}
	//	Gets previous link that uses this link as its source
	BaseType *GetAsSrcPrev()	{	return ( BaseType * ) srcPrev;	}
	//	Gets next link that uses this link as its source
	BaseType *GetAsSrcNext()	{	return ( BaseType * ) srcNext;	}

	//	Gets root of list of all links that use this link as their
      //	destination
	BaseType *GetAsDestRoot()	{	return ( BaseType * ) asDestRoot;	}
	//	Gets previous link that uses this link as its destination
	BaseType *GetAsDestPrev()	{	return ( BaseType * ) destPrev;	}
	//	Gets next link that uses this link as its destination
	BaseType *GetAsDestNext()	{	return ( BaseType * ) destNext;	}
};


/*-----------------------------------------------------------------------------
Class		MeshOf
                                                                           
Description	BasedLink is a templated shell for MeshLink used when all the
			links in a mesh have a common base class other than MeshLink
			itself.
			In homogenous meshes, where all the links are of type UserLink,
			UserLink could be declared as:
				class UserLink : public BasedLink<UserLink>

Template	LinkType		The type of link to be generated by and stored
							in the mesh.
                            Must be descended from MeshLink.
-----------------------------------------------------------------------------*/
//	LinkType must be descended from MeshLink
template <class LinkType>
class MeshOf : public Mesh
{
private:
	virtual MeshLink *CreateLink()
	{	return new LinkType;		}

public:
	LinkType *NewLink ( const string& label, const string& srcLabel, const string& destLabel )
	{	return ( LinkType * ) AddLink ( label, srcLabel, destLabel );	}

	LinkType *GetRoot()		{	return ( LinkType * ) root;				}

	LinkType *GetByLabel ( const string& label )
	{	return ( LinkType * ) FindByLabel ( label );	}

	LinkType *NewLink()
	{	
	  LinkType *link = (LinkType *)CreateLink();
	  assert(link!=NULL);
	  link->SetOwner(this);
	  return link;
	}

	LinkType *NewLink(const char *name)
	{
	  LinkType *link = NewLink();
	  link->SetLabel(name);
	  return link;
	}
};



//--Friend Function Prototypes-------------------------------------------------
//-----------------------------------------------------------------------------

#ifndef ROBOT

//	Inputs mesh from a stream (Concise format, Minimal content)
istream& operator >> ( istream& is, Mesh& mesh );

//	Outputs mesh to a stream (Prose format, Exhaustive content)
ostream& operator << ( ostream& os, Mesh& mesh );

#endif

#endif
