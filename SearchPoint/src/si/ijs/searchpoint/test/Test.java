package si.ijs.searchpoint.test;

import java.io.IOException;

import si.ijs.searchpoint.SearchPoint;

public class Test {

	public static void main(String[] args) throws IOException {
		try {
			System.loadLibrary("SearchPointJava");
		} catch (UnsatisfiedLinkError e) {
			e.printStackTrace();
			System.exit(1);
		}
		SearchPoint sp = SearchPoint.getInstance();
		
		int i = 0;
		while (i++ < 5) {
			try {
				Thread.sleep(1000);
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		
		sp.close();
	}
}
