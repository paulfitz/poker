/*--Module File Header---------------------------------------------------------

Module		mesh
                                                                           
Author		Paul Fitzpatrick, 4th Comp Eng, 9117261
Version		1.0
Date		15/11/94
Description This file defines classes to be used when setting up a Mesh
			data structure.

-----------------------------------------------------------------------------*/


//--Includes-------------------------------------------------------------------
//-----------------------------------------------------------------------------

#include <string.h>

#include "mesh.h"

//--Constants------------------------------------------------------------------
//-----------------------------------------------------------------------------

const char
	TabChar = '\x09';
const string
	NullName = "-";


//--Friend functions-----------------------------------------------------------
//-----------------------------------------------------------------------------


/*-----------------------------------------------------------------------------
Function	operator >>
                                                                           
Description	Loads a mesh from a stream.

Input		is				Input stream to read from.
			mesh			Mesh container to load to.

Returns		The input stream (for chained input)
-----------------------------------------------------------------------------*/
istream& operator >> ( istream& is, Mesh& mesh )
{
	mesh.Load ( is, Mesh::USER_INPUT_STYLE );

	return is;
}


/*-----------------------------------------------------------------------------
Function	operator <<
                                                                           
Description	Places a mesh on a stream.

Input		os				Output stream to write to.
			mesh			Mesh container to read from.

Returns		The output stream (for chained output)
-----------------------------------------------------------------------------*/
ostream& operator << ( ostream& os, Mesh& mesh )
{
	mesh.Store ( os, Mesh::USER_OUTPUT_STYLE );

	return os;
}


//--Class member functions-----------------------------------------------------
//-----------------------------------------------------------------------------

//--Class member functions-----------------------------------------------------
//--MeshLink-------------------------------------------------------------------

/*-----------------------------------------------------------------------------
Function	MeshLink
                                                                           
Description	Constructor. Initialises the MeshLink object.

Input		-

Returns		-
-----------------------------------------------------------------------------*/
MeshLink::MeshLink()
{
	src = dest = NULL;
	asSrcRoot = asDestRoot = NULL;
	srcPrev = srcNext = NULL;
	destPrev = destNext = NULL;
	meshPrev = meshNext = NULL;
	owner = NULL;
}


/*-----------------------------------------------------------------------------
Function	~MeshLink
                                                                           
Description	Destructor. Removes the MeshLink cleanly from the mesh - from
			anything it is connected to, or anything that connects to it.

Input		-

Returns		-
-----------------------------------------------------------------------------*/
MeshLink::~MeshLink()
{
	RemoveAsSource();
	RemoveAsDest();
	SetSource ( NULL );
	SetDest ( NULL );
	SetOwner ( NULL );
}


/*-----------------------------------------------------------------------------
Function	SetSource
                                                                           
Description	Sets the source link for the object.

Input		link			The link to connect to.		

Returns		-
-----------------------------------------------------------------------------*/
void MeshLink::SetSource ( MeshLink *link )
{
	if ( src != NULL )
	{
		//	Remove current source
		if ( src->asSrcRoot == this )	src->asSrcRoot = srcNext;
		if ( srcPrev != NULL )			srcPrev->srcNext = srcNext;
		if ( srcNext != NULL )			srcNext->srcPrev = srcPrev;
		srcPrev = srcNext = NULL;
		src = NULL;
	}
	if ( link != NULL )
	{
		//	Connect new source
		src = link;
		srcPrev = NULL;
		srcNext = link->asSrcRoot;
		if ( srcNext != NULL )	srcNext->srcPrev = this;
		link->asSrcRoot = this;
	}
}


/*-----------------------------------------------------------------------------
Function	SetDest
                                                                           
Description	Sets the destination link for the object.

Input		link			The link to connect to.

Returns		-
-----------------------------------------------------------------------------*/
void MeshLink::SetDest ( MeshLink *link )
{
	if ( dest != NULL )
	{
		//	Remove current destination
		if ( dest->asDestRoot == this )	dest->asDestRoot = destNext;
		if ( destPrev != NULL )			destPrev->destNext = destNext;
		if ( destNext != NULL )			destNext->destPrev = destPrev;
		destPrev = destNext = NULL;
		dest = NULL;
	}
	if ( link != NULL )
	{
		//	Connect new destination
		dest = link;
		destPrev = NULL;
		destNext = link->asDestRoot;
		if ( destNext != NULL )	destNext->destPrev = this;
		link->asDestRoot = this;
	}
}


/*-----------------------------------------------------------------------------
Function	SetOwner
                                                                           
Description	Sets the owner mesh for the object.

Input		mesh			The mesh to connect to.

Returns		-
-----------------------------------------------------------------------------*/
//had to kludge this - HIT fix it easily to not need a loop...
void MeshLink::SetOwner ( Mesh *mesh )
{
  MeshLink *cursor;
	if ( owner != NULL )
	{
		if ( owner->root == this )	owner->root = meshNext;
		if ( meshPrev != NULL )		meshPrev->meshNext = meshNext;
		if ( meshNext != NULL )		meshNext->meshPrev = meshPrev;
		meshPrev = meshNext = NULL;
		owner = NULL;
	}
	if ( mesh != NULL )
	{
		//	Connect to new owner
		owner = mesh;
		cursor = mesh->root;
		if ( cursor != NULL )
		  {
		    while ( cursor->meshNext != NULL )
		      {
			cursor = cursor->meshNext;
		      }
		    cursor->meshNext = this;
		  }
		else
		  {
		    mesh->root = this;
		  }
		meshPrev = cursor;
		meshNext = NULL;
	}
}


/*-----------------------------------------------------------------------------
Function	SetLabel
                                                                           
Description	Sets the label of the link.

Input		lbl				The new label of the link

Returns		-
-----------------------------------------------------------------------------*/
void MeshLink::SetLabel ( const string& lbl )
{
	label = lbl;
}

void MeshLink::SetLabel ( const char *lbl )
{
	label = lbl;
}


/*-----------------------------------------------------------------------------
Function	RemoveAsSource
                                                                           
Description	Disconnects any links that use this object as their source.

Input		-

Returns		-
-----------------------------------------------------------------------------*/
void MeshLink::RemoveAsSource()
{
	MeshLink
		*cursor = asSrcRoot;

	while ( cursor != NULL )
	{
		asSrcRoot = cursor->srcNext;
		cursor->srcPrev = NULL;
		cursor->srcNext = NULL;
		cursor->src = NULL;
		cursor = asSrcRoot;
	}
}


/*-----------------------------------------------------------------------------
Function	RemoveAsDest
                                                                           
Description	Disconnects any links that use this object as their destination.

Input		-

Returns		-
-----------------------------------------------------------------------------*/
void MeshLink::RemoveAsDest()
{
	MeshLink
		*cursor = asDestRoot;

	while ( cursor != NULL )
	{
		asDestRoot = cursor->destNext;
		cursor->destPrev = NULL;
		cursor->destNext = NULL;
		cursor->dest = NULL;
		cursor = asDestRoot;
	}
}


void MeshLink::Mark(int id)
{
  MeshLink *cursor;
  //  cout << "Marking " << GetLabel() << endl;
  if (pruneId != id)
    {
      pruneId = id;
      if (BaseGetSource() != NULL)
	{
	  BaseGetSource()->Mark(id);
	}
      if (BaseGetDest() != NULL)
	{
	  BaseGetDest()->Mark(id);
	}
      cursor = BaseGetAsSrcRoot();
      while (cursor != NULL)
	{
	  cursor->Mark(id);
	  cursor = cursor->BaseGetAsSrcNext();
	}
      cursor = BaseGetAsDestRoot();
      while (cursor != NULL)
	{
	  cursor->Mark(id);
	  cursor = cursor->BaseGetAsDestNext();
	}
    }
}



//--Class member functions-----------------------------------------------------
//--Mesh-----------------------------------------------------------------------


/*-----------------------------------------------------------------------------
Function	Mesh
                                                                           
Description	Constructor.

Input		-

Returns		-
-----------------------------------------------------------------------------*/
Mesh::Mesh()
{
	root = NULL;
	autoClear = -1;
}


/*-----------------------------------------------------------------------------
Function	Mesh
                                                                           
Description	Destructor. Wipes all links from the mesh.

Input		-

Returns		-
-----------------------------------------------------------------------------*/
Mesh::~Mesh()
{
	Clear();
}


/*-----------------------------------------------------------------------------
Function	FindByLabel
                                                                           
Description	Searches for a link with the given name within the mesh.

Input		label			The name to search for.

Returns		A pointer to the link if it was found, otherwise NULL
-----------------------------------------------------------------------------*/
MeshLink *Mesh::FindByLabel ( const string& label )
{
	int
		found = 0;
	MeshLink
		*cursor = root;

	while ( cursor != NULL && !found )
	{                              
		if ( label == cursor->GetLabel() )
		{
			found = 1;
		}
		else
		{
			cursor = cursor->BaseGetMeshNext();
		}
	}

	return cursor;
}


/*-----------------------------------------------------------------------------
Function	AddLink
                                                                           
Description	Adds a new link to the mesh

Input		label			The name of the new link
			srcLabel		The label of the link to use as its source
			destLabel		The label of the link to use as its destination

Returns		A pointer to the new link if it was created successfully,
			otherwise NULL.
			If any of the links named already exist, their current
            definition is used.
-----------------------------------------------------------------------------*/
MeshLink *Mesh::AddLink ( const string& label, const string& srcLabel, const string& destLabel )
{
	int
		success = 0;
	MeshLink
		*link,
		*srcLink,
		*destLink;

	link = LabelledLink ( label );
	srcLink = LabelledLink ( srcLabel );
	destLink = LabelledLink ( destLabel );

	if ( autoClear == -1 )
	{
		autoClear = 1;
	}

	if ( link != NULL )
	{
		link->SetSource ( srcLink );
		link->SetDest ( destLink );
		success = 1;
	}

	return success ? link : (MeshLink *) NULL;
}


/*-----------------------------------------------------------------------------
Function	LabelledLink
                                                                           
Description	Searches for a given link, and creates it if it isn't found

Input		label			The name of thelink

Returns		A pointer to the new if it was created successfully or found,
			otherwise NULL
-----------------------------------------------------------------------------*/
MeshLink *Mesh::LabelledLink ( const string& label )
{
	MeshLink
		*link = NULL;

	if ( !(label==NullName))
	{
		link = FindByLabel ( label );
		if ( link == NULL )
		{
			link = CreateLink();
			if ( link != NULL )
			{
				link->SetLabel ( label );
				link->SetOwner ( this );
			}
		}
	}

	return link;
}

#ifndef ROBOT

/*-----------------------------------------------------------------------------
Function	Load
                                                                           
Description	Loads a mesh from a stream.

Input		is				Input stream to read from.
			style			Style in which the mesh is assumed to be
							described.
							This is a combination of
								PROSE_FORMAT or CONCISE_FORMAT
							and
								MINIMAL_CONTENT or EXHAUSTIVE_CONTENT.

Returns		True if no error while loading, false on error
-----------------------------------------------------------------------------*/
int Mesh::Load ( istream& is, int style )
{
	char
		buf[256];
	int
		verbose = ( ( style & FORMAT ) == PROSE_FORMAT ),
		failed = 0,
		done = 0;
	string
		label,
		srcLabel,
		destLabel;
	MeshLink
		*link;

	do
	{
    	is >> buf;
		label = buf;

		if ( !(label == NullName ) )
		{
			if ( verbose )	is >> buf;		//	Eliminate "FROM"

			is >> buf;
			srcLabel = buf;
			if ( verbose )	is >> buf;		//	Eliminate "TO"

			is >> buf;
			destLabel = buf;
			if ( verbose )	is >> buf;		//	Eliminate "("

			link = AddLink ( label, srcLabel, destLabel );
			if ( link != NULL )
			{
				if ( !link->Read ( is, style ) )
				{
					//	Failed
					failed = 1;
				}
				else
            	{
					if ( verbose )	is >> buf;	//	Eliminate ")"
				}
			}
		}
		else
		{
			if ( verbose )
            {
				is >> buf;					//	Eliminate ":"
				is >> buf;					//	Eliminate "EndOfMesh"
            }
			done = 1;
		}
		if ( is.eof() )	{	done = 1;	}
	} while ( !done && !failed );

	return !failed;
}



/*-----------------------------------------------------------------------------
Function	Store
                                                                           
Description	Stores a mesh on a stream.

Input		os				Output stream to write to.
			style			Style in which the mesh is to be described.
							This is a combination of
								PROSE_FORMAT or CONCISE_FORMAT
							and
								MINIMAL_CONTENT or EXHAUSTIVE_CONTENT.

Returns		True if no error while storing, false on error
-----------------------------------------------------------------------------*/
int Mesh::Store ( ostream& os, int style )
{
	int
    	verbose = ( ( style & FORMAT ) == PROSE_FORMAT ),
		exhaustive = ( ( style & CONTENT ) == EXHAUSTIVE_CONTENT ),
		failed = 0;
	MeshLink
		*cursor = root;

	if ( cursor != NULL )
	{
		while ( cursor->BaseGetMeshNext() != NULL )
		{
			cursor = cursor->BaseGetMeshNext();
		}

		while ( cursor != NULL && !failed )
		{
			os << cursor->GetLabel() << TabChar;
			if ( verbose )	os << "FROM " << TabChar;
			if ( cursor->BaseGetSource() != NULL )
			{
				os << cursor->BaseGetSource()->GetLabel() << TabChar;
			}
			else
			{
				os << NullName << TabChar;
			}
			if ( verbose )	os << "TO " << TabChar;
			if ( cursor->BaseGetDest() != NULL )
			{
				os << cursor->BaseGetDest()->GetLabel() << TabChar;
			}
			else
			{
				os << NullName << TabChar;
			}

			if ( verbose )	os << "( ";
			if ( !cursor->Write ( os, style ) )
			{
				os << "< Couldn't write object in ";
				os << ( verbose ? "Prose-like" : "Concise" );
				os << " format with ";
				os << ( exhaustive ? "Exhaustive" : "Minimal" );
                os << " content >";

				failed = 1;
			}

			if ( verbose )		os << " )";
			os << endl;

			cursor = cursor->BaseGetMeshPrev();
		}

	}

	//	Terminate file
	if ( verbose )
	{
		os << NullName << " : EndOfMesh" << endl;
	}
	else
	{
		os << NullName;
	}

	return !failed;
}

#endif

/*-----------------------------------------------------------------------------
Function	Clear
                                                                           
Description	Deletes all links from a mesh.

Input		-
Returns		-
-----------------------------------------------------------------------------*/
void Mesh::Clear()
{
	MeshLink
		*cursor = root;

	if ( autoClear != 0 && autoClear != -1 )
	{
		while ( cursor != NULL )
		{
			cursor = root->BaseGetMeshNext();
			delete root;
			root = cursor;
		}
	}
}


void Mesh::Prune(MeshLink *link)
{
  MeshLink *cursor, *next;

  if (link != NULL && link->GetOwner() == this)
    {
      cursor = root;
      while (cursor != NULL)
	{
	  cursor->SetId(0);
	  cursor = cursor->BaseGetMeshNext();
	}

      link->Mark(1);

      cursor = root;
      while (cursor != NULL)
	{
	  next = cursor->BaseGetMeshNext();
	  if (cursor->GetId() == 0)
	    {
	      delete cursor;
	      cursor = NULL;
	    }
	  cursor = next;
	}
    }
}


int Mesh::Size()
{
  int count = 0;
  MeshLink *cursor;

  cursor = root;
  while (cursor != NULL)
    {
      count++;
      cursor = cursor->BaseGetMeshNext();
    }
  return count;
}
