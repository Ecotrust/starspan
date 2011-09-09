//
// Based on org.geotools.demos.data.ShapeReader
// simple non-gui program.
// Carlos Rueda
// $Id: ShapeDump.java,v 1.1 2005-02-19 22:37:58 crueda Exp $
//

package starspan.tools;

import com.vividsolutions.jts.geom.Geometry;
import org.geotools.data.FeatureReader;
import org.geotools.data.FeatureResults;
import org.geotools.data.FeatureSource;
import org.geotools.data.shapefile.ShapefileDataStore;
import org.geotools.feature.AttributeType;
import org.geotools.feature.Feature;
import org.geotools.feature.FeatureType;
import java.io.File;
import java.net.URL;

/**
 * Basic reading abilities demo: open a file, get the feature type, read the
 * features and output their contents to the standard output
 *
 * @author aaime
 */
public class ShapeDump {
    public static void main(String[] args) {
		if ( args.length == 0 ) {
			System.out.println("USAGE:  ShapeDump <shpfile>");
			return;
		}
        try {
			File f = new File(args[0]);
			URL shapeURL = f.toURL();

            // get feature results
            ShapefileDataStore store = new ShapefileDataStore(shapeURL);
            String name = store.getTypeNames()[0];
            FeatureSource source = store.getFeatureSource(name);
            FeatureResults fsShape = source.getFeatures();

            // print out a feature type header and wait for user input
            FeatureType ft = source.getSchema();
            System.out.println("FID\t");

            for (int i = 0; i < ft.getAttributeCount(); i++) {
                AttributeType at = ft.getAttributeType(i);

                if (!Geometry.class.isAssignableFrom(at.getType())) {
                    System.out.print(at.getType().getName() + "\t");
                }
            }

            System.out.println();

            for (int i = 0; i < ft.getAttributeCount(); i++) {
                AttributeType at = ft.getAttributeType(i);

                if (!Geometry.class.isAssignableFrom(at.getType())) {
                    System.out.print(at.getName() + "\t");
                }
            }

            System.out.println();

            // now print out the feature contents (every non geometric attribute)
            FeatureReader reader = fsShape.reader();

            while (reader.hasNext()) {
                Feature feature = reader.next();
                System.out.print(feature.getID() + "\t");

                for (int i = 0; i < feature.getNumberOfAttributes(); i++) {
                    Object attribute = feature.getAttribute(i);

                    if (!(attribute instanceof Geometry)) {
                        System.out.print(attribute + "\t");
                    }
                }

                System.out.println();
            }

            reader.close();
            System.out.println();
            System.out.println();
            System.out.println();

            // and finally print out every geometry in wkt format
            reader = fsShape.reader();

            while (reader.hasNext()) {
                Feature feature = reader.next();
                System.out.print(feature.getID() + "\t");
                System.out.println(feature.getDefaultGeometry());
                System.out.println();
            }

            reader.close();
        } catch (Exception e) {
            System.out.println("Ops! Something went wrong :-(");
            e.printStackTrace();
        }

        System.exit(0);
    }
}
