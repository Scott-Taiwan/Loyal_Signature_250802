CXX = g++
CXXFLAGS = -std=c++11 -Wall -O2
LDFLAGS = -lssl -lcrypto

TARGET = loyalty_signer

$(TARGET): loyalty_signer.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) loyalty_signer.cpp $(LDFLAGS)

clean:
	rm -f $(TARGET)
