#include "SparseMatrixCV.h"

#ifdef _DEBUG
#pragma comment(lib,"../x64/Debug/SparseMatrix.lib")
#else
#pragma comment(lib,"../x64/Release/SparseMatrix.lib")
#endif

#include <vector>

using namespace std;

SparseMatrixCV::SparseMatrixCV( int nrow, int ncol, const int index[][2], const double value[], int N )
	: SparseMatrix(0,0)
{
	struct Three{
		Three( int r, int c, double v) : row(r), col(c), val(v) { }
		int row, col;
		double val;
		inline bool operator<( const Three& t ) { return row < t.row || row==t.row && col < t.col; }
	};
	vector<Three> initdata; 
	for( int i=0; i<N; i++ ) {
		initdata.push_back( Three( index[i][0], index[i][1], value[i] ) ); 
	}

	std::sort( initdata.begin(), initdata.end() ); 


	double* non_zero_value  = new double[N]; 
	int* col_index  = new int[N]; 
	int* row_pointer = new int[nrow+1]; 

	int previous_row = -1; 
	for( int i=0; i<N; i++ ){
		non_zero_value[i] = initdata[i].val;
		col_index[i] = initdata[i].col;
		if( initdata[i].row != previous_row ) {
			row_pointer[++previous_row] = i; 
		}
	}
	row_pointer[nrow] = N; 

	// re-constuct the matrix with give data
	this->updateData( nrow, ncol, non_zero_value, col_index, row_pointer, N ); 

	// we don't need to release the data, the memroy is used in the matrix data
	//delete[] row_pointer;
	//delete[] col_index;
	//delete[] non_zero_value;
}


const cv::Mat_<double> operator*( const SparseMatrixCV& m1, const cv::Mat_<double>& m2 ){
	cv::Mat_<double> res( m1.row(), m2.cols ); 

	assert( m1.col()==m2.rows && "Matrix size does not match" ); 

	std::vector<double> res_nzval;
	std::vector<int> res_colidx;
	std::vector<int> res_rowptr;

	const double* const nzval1 = m1.data->getRow()->nzvel(); 
	const int* const colidx1   = m1.data->getRow()->colinx(); 
	const int* const rowptr1   = m1.data->getRow()->rowptr(); 
	
	// store the result as row-order
	res_rowptr.push_back( 0 ); 
	for( int r=0; r < m1.row(); r++ ) {
		for( int c=0; c < m2.cols; c++ ) {
			res(r,c) = 0.0; 
			for( int i = rowptr1[r]; i!=rowptr1[r+1]; i++ ) {
				res(r,c) += nzval1[i] * m2( colidx1[i], c );
			}
		}	
	}
	return res; 
}



SparseMatrixCV SparseMatrixCV::I( int rows )
{
	double* nzv = new double[rows];
	int* colind = new int[rows]; 
	int* rowptr = new int[rows+1];
	for( int i=0; i<rows; i++ ) {
		nzv[i] = 1.0;
		colind[i] = i; 
		rowptr[i] = i; 
	}
	rowptr[rows] = rows; 

	return SparseMatrixCV( rows, rows, nzv, colind, rowptr, rows ); 
}

void SparseMatrixCV::getRowMatrixData( int& N, double const** non_zero_value, int const** column_index, int const** row_pointer ) const 
{
	if( const SparseMatrixDataRow* rowData = this->data->getRow() ) {
		N = rowData->nnz();
		*non_zero_value = rowData->nzvel(); 
		*column_index = rowData->colinx(); 
		*row_pointer = rowData->rowptr(); 
	} else {
		N = 0;
		*non_zero_value = 0; 
		*column_index = 0; 
		*row_pointer = 0; 
	}
}

void SparseMatrixCV::convertTo( cv::Mat_<double>& m ){
	m = cv::Mat_<double>::zeros( this->row(), this->col() ); 

	if( this->data->getRow()==NULL ){
		return; 
	} 

	const int& N              = this->data->getRow()->nnz(); 
	const double* const nzval = this->data->getRow()->nzvel(); 
	const int* const colidx   = this->data->getRow()->colinx(); 
	const int* const rowptr   = this->data->getRow()->rowptr(); 
	
	int vi = 0; 
	for( int r=0; r < this->row(); r++ ) {
		for( int c=0; c < this->col(); c++ ) {
			if( colidx[vi]==c && vi<rowptr[r+1] ) 
				m(r,c) = nzval[vi++]; 
		}
	}
}