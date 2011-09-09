//
// Based on SpearfishSample.java (geotools)
// Carlos Rueda
// $Id: ShapeViewer.java,v 1.2 2008-02-06 10:06:36 crueda Exp $
//

package starspan.tools;

import java.util.List;
import java.util.ArrayList;
import java.util.Iterator;
import java.awt.Color;
import java.awt.Font;
import java.net.URL;
import javax.swing.JFrame;
import javax.swing.WindowConstants;
import java.io.File;

import org.geotools.data.FeatureSource;
import org.geotools.data.shapefile.ShapefileDataStore;
import org.geotools.feature.FeatureCollection;
import org.geotools.gui.swing.StyledMapPane;
import org.geotools.map.DefaultMapContext;
import org.geotools.map.MapContext;
import org.geotools.renderer.j2d.RenderedMapScale;
import org.geotools.styling.ColorMap;
import org.geotools.styling.Graphic;
import org.geotools.styling.LineSymbolizer;
import org.geotools.styling.Mark;
import org.geotools.styling.PointSymbolizer;
import org.geotools.styling.PolygonSymbolizer;
import org.geotools.styling.RasterSymbolizer;
import org.geotools.styling.Rule;
import org.geotools.styling.Style;
import org.geotools.styling.StyleBuilder;
import org.geotools.styling.Symbolizer;
import org.geotools.styling.TextSymbolizer;

/**
 * A simple viewer
 */
public class ShapeViewer {

    public static void main(String[] args) throws Exception {
        if ( args.length == 0 ) {
			System.err.println("missing args");
			return;
		}
		List list = new ArrayList();
		for ( int i = 0; i < args.length; i++ ) {
			File file = new File(args[i]);
			URL url = file.toURL();
			ShapefileDataStore dsRoads = new ShapefileDataStore(url);
			FeatureSource fsRoads = dsRoads.getFeatureSource(file.getName());
			list.add(fsRoads);
		}

        // Prepare styles
        StyleBuilder sb = new StyleBuilder();
        
        LineSymbolizer ls1 = sb.createLineSymbolizer(Color.YELLOW, 1);
        LineSymbolizer ls2 = sb.createLineSymbolizer(Color.BLACK, 5);
        Style roadsStyle = sb.createStyle();
        roadsStyle.addFeatureTypeStyle(sb.createFeatureTypeStyle(null, sb.createRule(ls2)));
        roadsStyle.addFeatureTypeStyle(sb.createFeatureTypeStyle(null, sb.createRule(ls1)));
        
        Mark redCircle = sb.createMark(StyleBuilder.MARK_CIRCLE, Color.RED, Color.BLACK, 0);
        Graphic grBugs = sb.createGraphic(null, redCircle, null);
        PointSymbolizer psBugs = sb.createPointSymbolizer(grBugs);
        Style bugsStyle = sb.createStyle(psBugs);
		
		
        // Build the map
        MapContext map = new DefaultMapContext();
		for ( Iterator it = list.iterator(); it.hasNext(); ) {
			FeatureSource fsRoads = (FeatureSource) it.next();
			map.addLayer(fsRoads, roadsStyle);
			map.addLayer(fsRoads, bugsStyle);
		}

        // Show the map
        StyledMapPane mapPane = new StyledMapPane();
        mapPane.setMapContext(map);
        mapPane.getRenderer().addLayer(new RenderedMapScale());
        JFrame frame = new JFrame();
        frame.setTitle("Delta");
        frame.setContentPane(mapPane.createScrollPane());
        frame.setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);
        frame.setSize(640, 480);
        frame.show();
    }
}
