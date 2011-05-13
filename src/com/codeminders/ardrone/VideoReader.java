
package com.codeminders.ardrone;

import java.awt.image.BufferedImage;
import java.io.IOException;
import java.net.*;
import java.nio.ByteBuffer;
import java.nio.channels.*;
import java.util.Iterator;
import java.util.Set;

import com.codeminders.ardrone.video.ImageDecoder;

public class VideoReader implements Runnable
{
    private static final int BUFSIZE = 4096;

    private DatagramChannel  channel;
    private ARDrone          drone;
    private Selector         selector;
    private boolean          done;

    public VideoReader(ARDrone drone, InetAddress drone_addr, int video_port) throws IOException
    {
        this.drone = drone;

        channel = DatagramChannel.open();
        channel.configureBlocking(false);
        channel.socket().bind(new InetSocketAddress(video_port));
        channel.connect(new InetSocketAddress(drone_addr, video_port));

        selector = Selector.open();
        channel.register(selector, SelectionKey.OP_READ | SelectionKey.OP_WRITE);
    }

    @Override
    public void run()
    {
        try
        {
            ByteBuffer inbuf = ByteBuffer.allocate(BUFSIZE);
            done = false;
            while(!done)
            {
                selector.select();
                if(done)
                {
                    disconnect();
                    break;
                }
                Set readyKeys = selector.selectedKeys();
                Iterator iterator = readyKeys.iterator();
                while(iterator.hasNext())
                {
                    SelectionKey key = (SelectionKey) iterator.next();
                    iterator.remove();
                    if(key.isWritable())
                    {
                        byte[] trigger_bytes = { 0x01, 0x00, 0x00, 0x00 };
                        ByteBuffer trigger_buf = ByteBuffer.allocate(trigger_bytes.length);
                        trigger_buf.put(trigger_bytes);
                        trigger_buf.flip();
                        channel.write(trigger_buf);
                        channel.register(selector, SelectionKey.OP_READ);
                    } else if(key.isReadable())
                    {
                        inbuf.clear();
                        int len = channel.read(inbuf);
                        byte[] packet = new byte[len];
                        inbuf.flip();
                        inbuf.get(packet, 0, len);

                        BufferedImage image = ImageDecoder.readUINT_RGBImage(packet);
                        drone.videoFrameReceived(image);
                    }
                }
            }
        } catch(Exception e)
        {
            drone.changeToErrorState(e);
        }

    }

    private void disconnect()
    {
        try
        {
            selector.close();
        } catch(IOException iox)
        {
            // Ignore
        }

        try
        {
            channel.disconnect();
        } catch(IOException iox)
        {
            // Ignore
        }
    }

    public void stop()
    {
        done = true;
        selector.wakeup();
    }

}