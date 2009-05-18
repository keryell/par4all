//     goal: show effect of cloning, partial evaluation and loop unrolling
//     and reduction parallelization for a Power architecture
//     kernel_size must be odd
#define image_size 512
#define kernel_size 3
#define nsteps 20

void convol();

int main() //      program image_processing
{
  float image[image_size][image_size];
  float new_image[image_size][image_size];
  float kernel[kernel_size][kernel_size];

  int i, j, n;

  for( i = 0; i< kernel_size; i++) {
    for( j = 0; j< kernel_size; j++) {
      kernel[i][j] = 1;
    }
  }

  //     read *, image
  for( i = 0; i< image_size; i++) {
    for( j = 0; j< image_size; j++)
      image[i][j] = 1.;
  }


  for( n = 0; n< nsteps; n++) {
    convol(image_size, image_size, new_image, image,
	   kernel_size, kernel_size, kernel);
      }

  //     print *, new_image
  //      print *, new_image (image_size/2, image_size/2)

  return 1; 
}

void convol(int isi, int isj, float new_image[isi][isj], float image[isi][isj], 
	    int ksi, int ksj, float kernel[ksi][ksj])
{
  //     The convolution kernel is not applied on the outer part
  //     of the image

  int i, j, ki, kj;
 
  for(i = 0; i< isi; i++) {
    for(j = 0; j< isj; j++) {
      new_image[i][j] = image[i][j];
    }
  }

 l400: for(i =  ksi/2; i<isi - ksi/2; i++) {
  l300: for(j =  ksj/2; j<isj - ksj/2; j++) {
      new_image[i][j] = 0.;
    l200: for(ki = 0; ki<ksi; ki++) {
      l100: for(kj = 0; kj<ksj; kj++) {
	  new_image[i][j] = new_image[i][j] + 
	    image[i+ki-ksi/2-1][j+kj-ksj/2-1]* 
	    kernel[ki][kj];
	}
      }
      new_image[i][j] = new_image[i][j]/(ksi*ksj);
    }
  }
}
