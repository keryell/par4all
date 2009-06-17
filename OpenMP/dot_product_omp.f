      PROGRAM DOT_PRODUCT

      INTEGER N, CHUNKSIZE, CHUNK, I
      PARAMETER (N=100)
      PARAMETER (CHUNKSIZE=10)
      REAL A(N), B(N), RESULT

!     Some initializations
!$OMP PARALLEL DO 
      DO I = 1, N
         A(I) = I*1.0
      ENDDO
!$omp end parallel do
!$OMP PARALLEL DO 
      DO I = 1, N
         B(I) = I*2.0
      ENDDO
!$omp end parallel do
      RESULT = 0.0
      CHUNK = CHUNKSIZE

!$omp parallel do
!$&reduction(+:RESULT)
      DO I = 1, N
         RESULT = RESULT+A(I)*B(I)
      ENDDO
!$omp end parallel do

      PRINT *, 'Final Result= ', RESULT
      END
