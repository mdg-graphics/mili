c $Id: f_util.f,v 1.7 2010/05/19 17:35:35 corey3 Exp $

c MILI Fortran utility functions

c ************************************************************************
c * Modifications:
c *
c *  I. R. Corey - May 19, 2010: Incorporated more changes from 
c *                Sebastian at IABG to support building under Windows.
c *                SCR #679.
c *
c ************************************************************************

      integer function EFF_LEN( string, limit )
      
      implicit none
      
      character*(*) string
      integer limit
      
      integer i
      
      if ( string(1:1) .eq. CHAR(0) ) then
          EFF_LEN = 0
      else
          i = LEN( string )
          if ( i .gt. limit ) i = limit
          do while ( ( i .ge. 1 ) .and. ( string(i:i) .eq. ' ' ) )
              i = i - 1
          end do
          
          EFF_LEN = i
      end if
      
      return
      end

c*********************************************************
c
c Replacement loop suggested from Sebastian @ IABG
c Addresses case where i=0.
c
c         do while ( i .ge. 1 )
c              if ( string(i:i) /= ' ' ) exit
c              i = i - 1
c          end do
c
c*********************************************************
