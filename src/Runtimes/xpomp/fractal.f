      program fractal

      include 'hpfc_graphic_F.h'
      
c     Algorithm parameters:
      integer n_iteration
      parameter (n_iteration = 256)
      real*8 big_value
      parameter (big_value = 100)
c     distorsion = 2 to have normal Mandelbrot:
      real*8 distorsion
      parameter (distorsion = 2)

c     Size of the iteration space:
      integer x_size, y_size
      parameter(x_size = 200)     
      parameter(y_size = 200)

c     The zooming ratio to display this iteration space:      
      integer x_display_zoom, y_display_zoom
      parameter(x_display_zoom = 2)     
      parameter(y_display_zoom = 2)

c     The zooming factor used when zooming with the button:
      real*8 zooming_factor
      parameter (zooming_factor = 4)


      integer x_display_size, y_display_size
      parameter(x_display_size = x_size*x_display_zoom)     
      parameter(y_display_size = y_size*y_display_zoom)
      
      integer k, x, y
      integer display
      character image(0:x_size - 1, 0:y_size - 1)
      
      real*8 zoom, xcenter, ycenter
      real*8 zr, zrp, zi, cr, ci
      integer status
      
      integer hpfc_open_display
      external hpfc_open_display
      integer hpfc_wait_mouse
      external hpfc_wait_mouse
      
      display = hpfc_open_display(x_display_size, y_display_size)
      status = hpfc_set_color_map(display, 1, 1, 0, 0)
      
      xcenter = 0
      ycenter = 0
      zoom = 5

      call hpfc_show_usage

      print *, 'Use mouse button 1 to zoom in, button 2 to recenter'
      print *, '    and button 3 to zoom out'

c     Main loop:
 10   continue

chpf$ independent     
      do 1 x = 0, x_size - 1
         cr = xcenter + (x - x_size/2)*zoom/x_size
chpf$ independent     
         do 2 y = 0, y_size - 1      
            ci = ycenter + (y - y_size/2)*zoom/y_size
            zr = cr
            zi = ci
           do 3 k = 0, n_iteration - 1
               d = zr*zr + zi*zi
               zrp = zr*zr - zi*zi + cr
               zi = distorsion*zr*zi + ci
               zr = zrp
               if (d .gt. big_value) then 
                goto 300
               endif
 3          continue
 300        continue
            image(x, y) = CHAR(k)
 2       continue
 1    continue
      
      status = hpfc_flash(display, image, x_size, y_size, 
     &     0, 0, x_display_zoom, y_display_zoom)
      
      button = hpfc_wait_mouse(display, x, y)
      xcenter = xcenter + (x/x_display_zoom - x_size/2)*zoom/x_size
      ycenter = ycenter + (y/y_display_zoom - y_size/2)*zoom/y_size
      if (button.eq.1) then
       zoom = zoom/zooming_factor
      else if (button.eq.3) then
       zoom = zoom*zooming_factor
      endif
      print *, 'Position (', xcenter, ',', ycenter, '), zoom =', zoom
      goto 10

      end
