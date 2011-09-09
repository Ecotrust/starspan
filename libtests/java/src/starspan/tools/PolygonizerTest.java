//
// PolygonizerTest
// $Id: PolygonizerTest.java,v 1.1 2005-02-19 22:37:58 crueda Exp $
//

package starspan.tools;

import com.vividsolutions.jts.io.WKTReader;
import com.vividsolutions.jts.io.WKTWriter;
import com.vividsolutions.jts.operation.polygonize.Polygonizer;
import com.vividsolutions.jts.geom.Geometry;
import com.vividsolutions.jts.geom.LineString;
import java.util.Collection;
import java.util.List;
import java.util.ArrayList;


/**
  * PolygonizerTest - 
  *
  * @author Carlos A. Rueda
  */
public class PolygonizerTest {
    public static void main(String[] args) throws Exception {
		test1();
	}

	static void test1()throws Exception {
		WKTReader r = new WKTReader();
		WKTWriter w = new WKTWriter();
		
		List lines = new ArrayList();
		lines.add(r.read("LINESTRING (0 0, 100 100)"));
		lines.add(r.read("LINESTRING (100 100, 100 0)"));
		lines.add(r.read("LINESTRING (100 0, 0 100)"));
		lines.add(r.read("LINESTRING (0 100, 0 0)"));
		
		Geometry noded = (LineString) lines.get(0);
		for ( int i = 1; i < lines.size(); i++ ) {
			noded = noded.union((LineString) lines.get(i));
		}
		
		System.out.println(w.write(noded));
		
		Polygonizer pp = new Polygonizer();
		pp.add(noded);
		
		Collection polys = pp.getPolygons();
		Collection cuts = pp.getCutEdges();
		Collection dangles = pp.getDangles();
		
		System.out.println(polys.size() + " polys obtained");
		System.out.println(cuts.size() + " cuts obtained");
		System.out.println(dangles.size() + " dangles obtained");
	}
}
