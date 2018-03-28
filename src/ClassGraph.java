/*
  ClassGraph.java

  Copyright (c) 1996 Roland Wunderling, Malte Zoeckler

  This file is part of DOC++.

  DOC++ is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public
  License as published by the Free Software Foundation; either
  version 2 of the license, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public
  License along with this program; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/* ==================================================================
 * N.B. IF YOU WANT TO CHANGE THIS FILE:
 *
 * This file is not actually compiled as part of the build process;
 * instead its .class file output is hard-coded into java.h.
 *
 * So, if you do change this file, you will also need to reconstruct
 * java.h.
 *
 * (TODO: Have a rule in Makefile.am to build java.h from the class
 * files.  Then ClassGraph.java can be removed from EXTRA_DIST, and
 * java.h removed from doc___SOURCES.)
 *
 * ================================================================== */

import java.applet.Applet;
import java.applet.AppletContext;
//import java.awt.BorderLayout;
import java.awt.*;
import java.util.StringTokenizer;
import java.net.URL;

class ClassLayout implements LayoutManager {

    public ClassLayout(){
    }

    public void addLayoutComponent(String name, Component comp) {
    }

    public void removeLayoutComponent(Component comp) {
    }
  
  int distance=30;
  int leftmax=2,rightmax=2;

    public Dimension preferredLayoutSize(Container parent) {
        Insets insets = parent.insets();
        int ncomponents = parent.countComponents();
        int w = 0;
        int h = ncomponents*distance;

        for (int i = 0; i < ncomponents; i++) {
            Component comp = parent.getComponent(i);
            Dimension d = comp.preferredSize();
            Point p = comp.location();
            if ((p.x + d.width) > w)
                w = p.x + d.width;
            if ((p.y + d.height) > h)
                h = p.y + d.height;
        }
	calcStuff(parent);
	w=(leftmax+rightmax)*distance;
        return new Dimension(insets.left + insets.right + w,
                             insets.top + insets.bottom + h);
    }

  void calcStuff(Container parent){
    int ncomponents = parent.countComponents();
        for (int i = 0; i < ncomponents; i++) {
	  NavigatorButton m=(NavigatorButton) parent.getComponent(i);
	  Dimension d = m.preferredSize();
	  int units=m.space+Math.max(2,(int)((d.width+distance-1)/distance));
	  if (m.growLeft){
	    if (units>leftmax)
	      leftmax=units;
	    } else 
	      if (units>rightmax)
		rightmax=units;
	}
  }

  public Dimension minimumLayoutSize(Container parent) {
    return preferredLayoutSize(parent);
  }
  boolean first=true;
    public void layoutContainer(Container parent) {
      calcStuff(parent);
      int     nmembers = parent.countComponents();
      /*System.out.println ("Hi, this is layout with # of comp="+nmembers+
	"left="+leftmax);    */
      int maxHeight=0;
      for (int i = 0; i < nmembers; i++) {
	NavigatorButton m=(NavigatorButton) parent.getComponent(i);
	Dimension d = m.preferredSize();
	if (d.height>maxHeight)
	  maxHeight=d.height;
      }	
      
      for (int i = 0; i < nmembers; i++) {
	NavigatorButton m=(NavigatorButton) parent.getComponent(i);
	Dimension d = m.preferredSize();
	if (d.width<2*distance)
	  d.width=2*distance;
	m.resize(d.width, d.height);
	Point l=m.location();
	// left border for right growing
	int left=leftmax-2;
	if (left<0) left=0;
	// right border for left growing
	int right=leftmax;

	if (m.growLeft)
	  l.x=(right-m.space)*distance-d.width;
	else 
	  l.x=(m.space+left)*distance;
	m.move(l.x,i*distance+(distance-maxHeight)/2);
	System.out.println (i+":"+m);    
      }
    }
}

class NavigatorButton extends Button{
  AppletContext context;
  URL url;
  int space,indent;
  // True if it grows to the left.
  boolean growLeft,bold,isInterface;
  Font boldFont,italicFont,normalFont;

  NavigatorButton(String name,URL _url,AppletContext _context,boolean gl,
		  boolean _isInterface, int _space,int _indent){
    super(name);
    isInterface=_isInterface;
    context=_context;
    url=_url;
    growLeft=gl;
    space=_space;
    indent=_indent;

    if (boldFont==null)
      boldFont=new Font("Helvetica",Font.BOLD, 12);
    if (italicFont==null)
      italicFont=new Font("Helvetica",Font.ITALIC, 12);
    if (isInterface) {
      setForeground(Color.blue);
      System.out.println (name+" is Interface");
    }
    
    if (url==null)
      setFont(italicFont);
      
    if (space==0){
      setFont(boldFont);
    }
  }

  public void init(){
    System.out.println ("Font="+getFont());
  }
  /*
  public String toString(){
    return("Button "+url+
	   " space="+space+" rect="+bounds());
  }
  */
  public boolean action(Event evt, Object arg) {
    if (url!=null){
      try {
	context.showDocument(url);
      }  catch (Exception e) {};
      return(true);
    }
    return(false);
  }
}

class ClassGraphPanel extends Panel{
  
  int ccnt=0;
  public Button buttons[];
  public String urls[],before[],after[];
  public static String dir;

  int indent[];
  ClassLayout layout;
  ClassGraphPanel(String _classes,String _indent,String _before,String _after,
		  AppletContext context,URL base){

    StringTokenizer tc = new StringTokenizer(_classes, ",");
    StringTokenizer tb = new StringTokenizer(_before, ",");
    StringTokenizer ti = new StringTokenizer(_indent, ",");
    StringTokenizer ta = new StringTokenizer(_after, ",");

    buttons=new Button[tb.countTokens()];
    urls=new String[tb.countTokens()];
    before=new String[tb.countTokens()];
    after=new String[tb.countTokens()];
    indent=new int[tb.countTokens()];
    for (; tc.hasMoreTokens();) {
      String tmp   = tc.nextToken();
      
      String type  = tmp.substring(0,1);
      String cname = tmp.substring(1);

      String file  = tc.nextToken().substring(1);
      before[ccnt] = tb.nextToken().substring(1);
      after[ccnt]  = ta.nextToken().substring(1);
      /*System.out.println ("Cnt="+ccnt+" name="+cname+
	" url="+file+"\nafter="+after[ccnt]+" before="+
	before[ccnt]);*/
      indent[ccnt]=Integer.valueOf(ti.nextToken()).intValue();
      URL url=null;
      if (file.length()>0)
	try {
	  url=new URL(base,file);
	} catch(Exception e) {};
	boolean growLeft=false;
	if (before[ccnt].length()<2)
	  growLeft=true;
	int space=(after[ccnt].length()+before[ccnt].length())/2;
	boolean isInterface=true;
	if (type.equals("C"))
	  isInterface=false;
	buttons[ccnt]=new NavigatorButton(cname,url,context,growLeft,isInterface,
					  space,indent[ccnt]);
	ccnt++;
    }

    setLayout(layout=new ClassLayout());
    for (int i=0 ; i< ccnt ; i++){
      add(buttons[i]);
      buttons[i].move(30*(indent[i]+before[i].length()/2),i*30);
    }
    //System.out.println ("Size:"+buttons[0].size());    
  }

  int h=30,w=30;
  void rightArrow(Graphics g,int x,int y){
    if (dir.equals("up"))
	return;
    g.drawLine(x-4, y-3,x,y);
    g.drawLine(x-4, y+3,x,y);
  }
  void downArrow(Graphics g,int x,int y){
    if (dir.equals("up"))
	return;
    g.drawLine(x-3, y-4,x,y);
    g.drawLine(x+3, y-4,x,y);
  }
  void leftArrow(Graphics g,int x,int y){
    if (dir.equals("down"))
	return;
    g.drawLine(x+4, y-3,x,y);
    g.drawLine(x+4, y+3,x,y);
  }
  void upArrow(Graphics g,int x,int y){
    if (dir.equals("down"))
	return;
    g.drawLine(x-3, y+4,x,y);
    g.drawLine(x+3, y+4,x,y);
  }
 
  void paintLine(Graphics g,int x,int y,String type){

    int dh=(h-buttons[0].size().height+1)/2;    
    for (int i=0 ; i<type.length()-1 ; i+=2){
      String sub=type.substring(i,i+1);
      String prot=type.substring(i+1,i+2);

      if (sub.equals("|")){
	g.drawLine(x+w/2, y-dh ,x+w/2,y+h);
	//if (!upArrows)
	if (!prot.equals("|")){
	  g.drawLine(x+w/2, y+h/2 ,x+w,y+h/2);
	  rightArrow(g,x+w-1,y+h/2);
	  upArrow(g,x+w/2,y-dh+1);
	}
      } else if (sub.equals("r")){
	g.drawLine(x+w/2, y ,x+w/2,y+h/2);
	g.drawLine(x+w/2, y+h/2 ,x+w,y+h/2);
	rightArrow(g,x+w-1,y+h/2);
      } else if (sub.equals("l")){
	g.drawLine(x, y+h/2 ,x+w/2,y+h/2);
	g.drawLine(x+w/2, y+h/2 ,x+w/2,y+h);
	leftArrow(g,x+1,y+w/2);
      } else if (sub.equals("L")){
	g.drawLine(x, y+h/2 ,x+w/2,y+h/2);
	g.drawLine(x+w/2, y ,x+w/2,y+h);
	leftArrow(g,x+1,y+w/2);
      } else if (sub.equals("R")){
	g.drawLine(x+w/2, y+h/2 ,x+w,y+h/2);
	g.drawLine(x+w/2, y ,x+w/2,y+h);
	rightArrow(g,x+w-1,y+h/2);
      } else if (sub.equals("D")){
	g.drawLine(x, y+h/2 ,x+w/2,y+h/2);
	g.drawLine(x+w/2, y ,x+w/2,y+h);
	downArrow(g,x+w/2, y+h);
	leftArrow(g,x+1,y+w/2);
      } else if (sub.equals("d")){
	g.drawLine(x, y+h/2 ,x+w/2,y+h/2);
	g.drawLine(x+w/2, y+h/2 ,x+w/2,y+h);
	downArrow(g,x+w/2, y+h);
	leftArrow(g,x+1,y+w/2);
      } else if (sub.equals("^")){
	g.drawLine(x+w/2, y - dh ,x+w/2,y+h/2);
	g.drawLine(x+w/2, y+h/2 ,x+w,y+h/2);
	rightArrow(g,x+w-1,y+h/2);
	upArrow(g,x+w/2,y-dh+1);
      }
      x+=w;      
    }
  }
 boolean first=true;
public void paint(Graphics g)
  { 
    int x,y;
    // 24 pt red font used here
    /*g.setFont(new Font("TimesRoman", Font.BOLD, 24));
      String test=new String("Malte");
      g.drawString(test, 0,40);
      */
      g.setColor(Color.black);
      for (int i=0 ; i<ccnt ; i++){
      x=(Math.max((layout.leftmax-2),0))*30;
      /*if (first)
	System.out.println ("x:"+x);*/
      
      y=i*30;
      paintLine(g,x,y,before[i]);
      Rectangle r=buttons[i].bounds();
      x=r.x+r.width;
      paintLine(g,x,y,after[i]);
    }
    first=false;
  }
}

public class ClassGraph extends Applet{
  ClassGraphPanel panel;

  public void init() {
    String classes = getParameter("classes");
    String before = getParameter("before");
    String after = getParameter("after");
    String indent = getParameter("indent");
    ClassGraphPanel.dir = getParameter("arrowdir");

    setLayout(new FlowLayout(FlowLayout.LEFT));
    //setLayout(new GridLayout(0,1,0,1));

    panel = new ClassGraphPanel(classes,indent,before,after,
				getAppletContext(),getDocumentBase());
    //add("Center", panel);
    add(panel);
    resize(panel.preferredSize());    
    setBackground(new Color(0xffffff)) ;
  }
}
