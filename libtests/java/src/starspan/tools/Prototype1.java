//
// Prototype1 - A first prototype working on a shapefile and a Envi header
// Carlos Rueda
// $Id: Prototype1.java,v 1.2 2008-02-06 10:06:36 crueda Exp $
//

package starspan.tools;

import envifile.EnviStandardFile;

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
  * Prototype1 - A first prototype working on a shapefile and a raster header.
  *
  * @author Carlos A. Rueda
  * @author (based on GeoTools ShapeReader)
  */
public class Prototype1 {
	/** Main entry for this prototype. */
    public static void main(String[] args) throws Exception {
		if ( args.length < 1 ) {
			System.out.println(
				"starspan - prototype 1\n" +
				"USAGE:  starspan <shpfile>  [<raster-header>]"
			);
			return;
		}
		String shp_filename = args[0];
		read_shape(shp_filename);

		if ( args.length > 1 ) {
			String hdr_filename = args[1];
			read_header(hdr_filename);
		}
	}
	
	private static void read_shape(String shp_filename) throws Exception {
		File f = new File(shp_filename);
		URL shapeURL = f.toURL();

		ShapefileDataStore store = new ShapefileDataStore(shapeURL);
		String name = store.getTypeNames()[0];
		FeatureSource source = store.getFeatureSource(name);
		FeatureResults fsShape = source.getFeatures();

		if ( true ) {
            // print out a feature type header
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

			FeatureReader reader = fsShape.reader();

			if ( false ) {
				// now print out the feature contents (every non geometric attribute)
	
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
			}

            reader.close();

            // and finally print out every geometry in wkt format
            reader = fsShape.reader();

            while (reader.hasNext()) {
                Feature feature = reader.next();
                System.out.print(feature.getID() + "\t");
                System.out.println(feature.getDefaultGeometry());
                System.out.println();
            }

            reader.close();
        }
    }

	private static void read_header(String hdr_filename) throws Exception {
		EnviStandardFile.Header header = EnviStandardFile.readHeader(hdr_filename);
		System.out.println("raster header:\n" + header.map);
	}
}
