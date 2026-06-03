#pragma once

#include "define.h"
#include <map>

using namespace std;

#define CSRVMIN_SIZE	(16)



class Permutation
{
public:
	Permutation(int* perm, int len)
	{
		m_perm = perm;

		for (int i = 0; i < len; i++)
		{
			m_invperm.at( m_perm[i]) =  i ;
		}
	};
	~Permutation(){};
public:
	int* m_perm;
	map<int,  int > m_invperm;
};


class Vec
{
public:
	 /** Return an array corresponding to the vector's elements. This
     * may or may not be the internal representation of the vector, so
     * callers should not modify this array.
	 **/
	//public abstract double[] getDoubles();
	virtual void getDoubles( map<int,  double>& pData ){};

    /** How long is the vector? **/
    //public abstract int size();
	virtual int size(){ return 0; };

    /** How many non-zero entries are there? **/
    //public abstract int getNz();
	virtual int getNz(){ return 0; };

    /** Get the element at index idx **/
    //public abstract double get(int idx);
	virtual double get( int idx ){ return 0.0; };

    /** Set the element at index idx to v. */
    //public abstract void set(int idx, double v);
	virtual void set( int idx, double data ){};

    /** Compute the dot product with vector r **/
    //public abstract double dotProduct(Vec r);
	virtual double dotProduct( Vec r ){ return 0.0; };
	//virtual double dotProduct( CSRVec r ){ return 0.0; };

    // dot product from [i0, i1]
    //public abstract double dotProduct(Vec r, int i0, int i1);
	virtual double dotProduct( Vec r, int i0, int i1 ){ return 0.0; };
	//virtual double dotProduct( CSRVec r, int i0, int i1 ){ return 0.0; };

    /** Make a copy of the vector **/
    //public abstract Vec copy();
	virtual Vec copy(){ Vec dateV; return dateV; };

    //public abstract double[] copyArray();
	virtual void copyArray( map<int, double>& pData ){};

    /** Resize the vector, truncating or adding zeros as appropriate. **/
    //public abstract void resize(int newlength);
	virtual void resize( int newlength ){};

    /** create a new, smaller vector beginning at element i0, going
	through i1 (inclusive). The length of this vector will be i1-i0+1.
    **/
    //public abstract Vec copy(int i0, int i1);
	//virtual Vec copy(int i0, int i1){ Vec dateV; return dateV; };

    /** create a same-sized vector containing only the spec'd elements. **/
    //public abstract Vec copyPart(int i0, int i1);
	virtual Vec copyPart( int i0, int i1 ){ Vec dateV; return dateV; };

    /** Multiply all elements in the vector by v **/
    //public abstract void timesEquals(double v);
	virtual void timesEquals( double v ){};

    /** Multiply the elements between indices [i0,i1] (inclusive) by
     * v **/
    //public abstract void timesEquals(double v, int i0, int i1);
	virtual void timesEquals( double v, int i0, int i1 ){};

    /** Set all elements to zero. **/
    //public abstract void clear();
	virtual void clear(){};

    /** Add the value v to each element. **/
 //   public void plusEquals(int idx, double v)
 //   {
	//set(idx, get(idx) + v);
 //   }
	void plusEquals( int idx, double v )
	{
		set(idx, get(idx)+v);
	};

    /** Add the vector v to the elements beginning at index idx **/
 //   public void plusEquals(int idx, double v[])
 //   {
	//for (int i = 0; i < v.length; i++)
	//    set(idx+i, get(idx+i) + v[i]);
 //   }
	void plusEquals( int idx, double* v, int len )
	{
		for( int i=0; i<len; i++ )
		{
			set(idx+i, get(idx+i)+v[i]);
		}
	};

    /** sum of squared elements. **/
    //public abstract double normF();
	virtual double normF(){ return 0.0; };

    /** Insert this vector as column 'col' in matrix A. The column is
	initially all zero. The vector should iterate through its
	elements, calling the matrix's set method.
    **/
    //public abstract void transposeAsColumn(Matrix A, int col);
	virtual void transposeAsColumn(/*CMatrix*/LPVOID A, int col){};

    /** Transpose only the elements at indices [i0,i1] inclusive. **/
    //public abstract void transposeAsColumn(Matrix A, int col, int i0, int i1);
	virtual void transposeAsColumn(/*CMatrix*/LPVOID A, int col, int i0, int i1){};

    /** Add this vector (scaled) to another vector. This vector
	is unchanged.
    **/
    //public abstract void addTo(Vec r, double scale);
    //public abstract void addTo(Vec r, double scale, int i0, int i1);
	virtual void addTo(Vec r, double scale){};
	virtual void addTo(Vec r, double scale, int i0, int i1){};

    /** reorder the columns of this matrix so that they are:
	X' = [ X(perm[0]) X(perm[1]) X(perm[2])... ]
    **/
    //public abstract Vec copyPermuteColumns(Permutation p);
	virtual Vec copyPermuteColumns(Permutation& p){ Vec dateV; return dateV; };
};

class CSRVec : virtual public Vec
{
public:
	CSRVec(){ m_pIndices=NULL; m_pValue = NULL; };
	CSRVec(int length, int capacity=CSRVMIN_SIZE);
	~CSRVec();

private:
	int m_lastGetIdx;
	int m_lastSetIdx;

	void grow( int count );
public:
	int m_length;		// logical length of the vector
	int m_capacity;
	int* m_pIndices;
	double* m_pValue;
	int m_nz;			// how many elements of indices/values are valid?

	Vec copy();
	void copy( CSRVec* pVec );
	void copyArray( map<int,  double>& pData );
	void getDoubles(map<int, double>& pData );
	void filterZeros();
	void filterZeros( double eps );
	void insert(int i, int idx, double v);
	void grow();
	void resize(int newlength);
	void sort();
	void ensureCapacity(int mincapacity);
	Vec copy(int i0, int i1);
	CSRVec copyPart1(int i0, int i1);
	void copyPart1 (int i0, int i1, CSRVec* pVec);
	int size();
	int getNz();
	double get( int idx );
	double getRef(int idx);
	void set(int idx, double v);
	void setRef( int idx, double v );
	double dotProduct( CSRVec r );
	double dotProduct( CSRVec r, int i0, int i1 );
	void timesEquals(double scale);
	void timesEquals(double scale, int i0, int i1);
	void transposeAsColumn(/*CMatrix*/LPVOID A, int col);
	void transposeAsColumn(/*CMatrix*/LPVOID A, int col, int i0, int i1);
	void add(CSRVec csra, double ascale, CSRVec csrb, int i0, int i1, CSRVec& res);
	void addTo(CSRVec& r, double scale);
	void addTo(CSRVec& r, double scale, int i0, int i1);
	void plusEquals(int idx, double v);
	void clear(){m_nz = 0;};
	double normF();
	CSRVec copyPermuteColumns1(Permutation& p);
};

class CDenseVec : virtual public Vec
{
public:
	CDenseVec( int length );
	CDenseVec( double* pValue, int length );
	CDenseVec(){ m_capacity=1;m_pValue = new double[1];};
	~CDenseVec();

public:
	int m_capacity;
	double* m_pValue;

	Vec copy();
	void copy( CDenseVec* pVec );
	void copyArray( map<int,  double>& pData );
	Vec copy(int i0, int i1);
	CDenseVec copyPart1(int i0, int i1);
	void copyPart1 (int i0, int i1, CDenseVec* pVec);
	void resize(int newlength);
	bool getDoubles( int length, double* pValue );
	int size();
	int getNz();
	double get( int idx );
	void set(int idx, double v);
	double dotProduct(CDenseVec* r);
	double dotProduct(CDenseVec* r, int i0, int i1);
	void timesEquals(double scale);
	void timesEquals(double scale, int i0, int i1);
	void transposeAsColumn(/*CMatrix*/LPVOID A, int col);
	void transposeAsColumn(/*CMatrix*/LPVOID A, int col, int i0, int i1);
	void addTo(CDenseVec& r, double scale);
	void addTo(Vec& r, double scale, int i0, int i1);
	void clear(){
		for (int i = 0; i < m_capacity; i++)
			m_pValue[i] = 0;
	};
	double normF();
	CDenseVec copyPermuteColumns1(Permutation& p);
};
