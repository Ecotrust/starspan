/**
 * @opt all
 * @opt types
 * @hidden
 */
class UMLOptions {}

/** @hidden */  class Object {}
/** @hidden */  class Objects {}
/** @hidden */  class OGRLayer {}
/** @hidden */  class GDALDataset {}
/** @hidden */  class OGRFeature {}
/** @hidden */  class OGREnvelope {}
/** @hidden */  class GDALDataType {}
/** @hidden */  class Signature {}
/** @hidden */  class DBFHandle {}
/** @hidden */  class FILE {}
/** @hidden */  class GDALRasterBand {}
/** @hidden */  class OGRPolygon {}
/** @hidden */  class GlobalInfo { }


class Raster {
	public GDALDataset getDataset();
	public void getSize(Objects __);
	public void getCoordinates(Objects __);
	public void getPixelSize(Objects __);
	public Object getBandValuesForPixel(int col, int row);
	public void toColRow(double x, double y, Objects __);
	public void report(FILE file);
}

class Vector {
	public String getName();
	public int getLayerCount();
	public OGRLayer getLayer(int layer_num);
	public void report(FILE file);
}


interface LineRasterizerObserver {
	void pixelFound(double x, double y);
}
/**
  * @navassoc - - 0..1 LineRasterizerObserver 
  */
class LineRasterizer {
	public void setObserver(LineRasterizerObserver obs);
	public void line(double x1, double y1, double x2, double y2, boolean last);
}

/**
  * @has 1  - 1..*  Raster 
  * @has 1  - 1  Vector 
  * @navassoc - - 0..* Observer 
  */
class Traverser implements LineRasterizerObserver {
	public void setVector(Vector vector);
	public Vector getVector();
	public void addRaster(Raster r);
	public int getNumRasters();
	public Raster getRaster(int i);
	public void setPixelProportion(double pixprop);
	public void setDesiredFID(long FID);
	public void addObserver(Observer obs);
	public Object getBandValuesForPixel(int col, int row, Object buffer);
	public void traverse();
	void pixelFound(double x, double y);
}

class TraversalEvent {
	public int col;
	public int row;
	public double x;
	public Object bandValues;
}



/**
  * @navassoc - <uses> - TraversalEvent
  */
interface Observer {
	boolean isSimple();
	void init(GlobalInfo info);
	void intersectionFound(OGRFeature feature);
	void addPixel(TraversalEvent ev);
	void end();
}


class CSVObserver implements Observer {}
class DBObserver implements Observer {}
class EnviSlObserver implements Observer {}
class StatsObserver implements Observer {}
class DumpObserver implements Observer {}
class JtsTestObserver implements Observer {}

