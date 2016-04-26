//
//  main.cpp
//  linelessdiff
//
//  Created by Uli Kusterer on 26/04/16.
//  Copyright © 2016 Uli Kusterer. All rights reserved.
//

#include <iostream>
#include <vector>


using namespace std;


const char*	difference_types[] = { "SAME", "ADDED", "REMOVED", "CHANGED" };


class difference
{
public:
	enum difference_type
	{
		SAME = 0,
		ADDED,
		REMOVED,
		CHANGED
	};

	difference() : mType(SAME), mOriginalStart(0), mOriginalLength(0), mModifiedStart(0), mModifiedLength(0), mOriginal(nullptr), mModified(nullptr) {}
	void	print() const
	{
		string stro( mOriginal + mOriginalStart, mOriginalLength );
		string strm( mModified + mModifiedStart, mModifiedLength );
		cout << difference_types[mType] << " (" << mOriginalStart << ", " << mOriginalLength << ") (" << mModifiedStart << ", " << mModifiedLength << ") \"" << stro << "\" \"" << strm << "\"" << endl;
	}

	const char*				mOriginal;
	const char*				mModified;
	enum difference_type	mType;
	size_t					mOriginalStart;
	size_t					mOriginalLength;
	size_t					mModifiedStart;
	size_t					mModifiedLength;
};


void	determine_identicals( const char* inOriginal, size_t inOriginalLen, const char* inModified, size_t inModifiedLen, std::vector<difference>* inDifferences )
{
	difference		currDifference;
	currDifference.mOriginal = inOriginal;
	currDifference.mModified = inModified;
	
	for( size_t xo = 0; xo < inOriginalLen; xo++ )
	{
		for( size_t xm = 0; xm < inModifiedLen; xm++ )
		{
			currDifference.mType = difference::difference_type::SAME;
			currDifference.mModifiedStart = xm;
			currDifference.mOriginalStart = xo;
			currDifference.mModifiedLength = 0;
			currDifference.mOriginalLength = 0;
			
			for( size_t n = 0; ((xo +n) < inOriginalLen) && ((xm +n) < inModifiedLen); n++ )
			{
				if( inOriginal[xo + n] == inModified[xm + n] )
				{
					currDifference.mModifiedLength = n +1;
					currDifference.mOriginalLength = n +1;
				}
				else
					break;
			}
			
			if( currDifference.mModifiedLength > 0 && currDifference.mOriginalLength > 0 )
			{
				inDifferences->push_back( currDifference );
			}
		}
	}

	sort( inDifferences->begin(), inDifferences->end(), []( const difference& left, const difference& right )
	{
		return (right.mOriginalLength < left.mOriginalLength);
	} );
	
	size_t		lastOriginalOffset = 0, lastModifiedOffset = 0;
	for( auto bigger = inDifferences->begin(); bigger != inDifferences->end(); )
	{
		for( auto smaller = inDifferences->begin(); smaller != inDifferences->end(); )
		{
			if( smaller == bigger )
			{
				lastOriginalOffset = smaller->mOriginalStart +smaller->mOriginalLength;
				lastModifiedOffset = smaller->mModifiedStart +smaller->mModifiedLength;
				smaller++;
				continue;
			}
			
			size_t	skippedBeforeo = smaller->mOriginalStart -bigger->mOriginalStart;
			size_t	skippedBeforem = smaller->mModifiedStart -bigger->mModifiedStart;
			if( smaller->mOriginalStart >= bigger->mOriginalStart
				&& (smaller->mOriginalLength +skippedBeforeo) <= bigger->mOriginalLength )
			{
				smaller = inDifferences->erase(smaller);
			}
			else if( smaller->mModifiedStart >= bigger->mModifiedStart
				&& (smaller->mModifiedLength +skippedBeforem) <= bigger->mModifiedLength )
			{
				smaller = inDifferences->erase(smaller);
			}
			else if( smaller->mOriginalStart < lastOriginalOffset )
				smaller = inDifferences->erase(smaller);
			else if( smaller->mModifiedStart < lastModifiedOffset )
				smaller = inDifferences->erase(smaller);
			else
			{
				lastOriginalOffset = smaller->mOriginalStart +smaller->mOriginalLength;
				lastModifiedOffset = smaller->mModifiedStart +smaller->mModifiedLength;
				smaller++;
			}
		}
		
		bigger++;
	}
	
	// Add a last faux "identical" entry that indicates the end-of-file so that
	//	mark_up_differences() doesn't swallow the last change/difference:
	currDifference.mType = difference::difference_type::SAME;
	currDifference.mOriginalStart = inOriginalLen;
	currDifference.mOriginalLength = 0;
	currDifference.mModifiedStart = inModifiedLen;
	currDifference.mModifiedLength = 0;
	inDifferences->push_back( currDifference );
}


void	mark_up_differences( const vector<difference>& differences, vector<difference>* inDifferences )
{
	size_t	lastOriginalOffset = 0, lastModifiedOffset = 0;
	for( const difference& currDifference : differences )
	{
		if( lastOriginalOffset < currDifference.mOriginalStart
			&& lastModifiedOffset < currDifference.mModifiedStart)	// Something between last identical spot and start of this one in both versions --> change!
		{
			difference	changeDifference;
			changeDifference.mType = difference::difference_type::CHANGED;
			changeDifference.mOriginal = currDifference.mOriginal;
			changeDifference.mOriginalStart = lastOriginalOffset;
			changeDifference.mOriginalLength = currDifference.mOriginalStart -lastOriginalOffset;
			changeDifference.mModified = currDifference.mModified;
			changeDifference.mModifiedStart = lastModifiedOffset;
			changeDifference.mModifiedLength = currDifference.mModifiedStart -lastModifiedOffset;
			inDifferences->push_back(changeDifference);
		}
		else if( lastOriginalOffset < currDifference.mOriginalStart )	// Something between last identical spot and start of this one in original --> deletion!
		{
			difference	deletionDifference;
			deletionDifference.mType = difference::difference_type::ADDED;
			deletionDifference.mOriginal = currDifference.mOriginal;
			deletionDifference.mOriginalStart = lastOriginalOffset;
			deletionDifference.mOriginalLength = currDifference.mOriginalStart -lastOriginalOffset;
			deletionDifference.mModified = currDifference.mModified;
			deletionDifference.mModifiedStart = lastModifiedOffset;
			deletionDifference.mModifiedLength = 0;
			inDifferences->push_back(deletionDifference);
		}
		else if( lastModifiedOffset < currDifference.mModifiedStart )	// Something between last identical spot and start of this one in modified version --> addition
		{
			difference	additionDifference;
			additionDifference.mType = difference::difference_type::REMOVED;
			additionDifference.mOriginal = currDifference.mOriginal;
			additionDifference.mOriginalStart = lastOriginalOffset;
			additionDifference.mOriginalLength = 0;
			additionDifference.mModified = currDifference.mModified;
			additionDifference.mModifiedStart = lastModifiedOffset;
			additionDifference.mModifiedLength = currDifference.mModifiedStart -lastModifiedOffset;
			inDifferences->push_back(additionDifference);
		}
		lastOriginalOffset = currDifference.mOriginalStart + currDifference.mOriginalLength;
		lastModifiedOffset = currDifference.mModifiedStart + currDifference.mModifiedLength;
		inDifferences->push_back(currDifference);
	}
}


void	determine_differences( const char* inOriginal, size_t inOriginalLen, const char* inModified, size_t inModifiedLen, std::vector<difference>* inDifferences )
{
	vector<difference>	differences;
	determine_identicals( inOriginal, inOriginalLen, inModified, inModifiedLen, &differences );
	mark_up_differences( differences, inDifferences );
}


void	print_differences( const vector<difference>& differences )
{
	for( const difference& currDifference : differences )
	{
		currDifference.print();
	}
}


int main(int argc, const char * argv[])
{
	const char*		original = "Well, there you go.";
	const char*		modified = "There you go, dork";
	
	cout << original << endl;
	cout << modified << endl;
	vector<difference>	differences;
	determine_differences( original, strlen(original), modified, strlen(modified), &differences );
	print_differences( differences );
	cout << endl;
	
	cout << original << endl;
	cout << modified << endl;
	differences.resize(0);
	original = "Well, there you go.";
	modified = "There you go.";
	determine_differences( original, strlen(original), modified, strlen(modified), &differences );
	print_differences( differences );
	cout << endl;

	cout << original << endl;
	cout << modified << endl;
	differences.resize(0);
	original = "There you go.";
	modified = "There you go.";
	determine_differences( original, strlen(original), modified, strlen(modified), &differences );
	print_differences( differences );
	
    return 0;
}
