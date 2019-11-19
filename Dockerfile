FROM python:3.6
MAINTAINER gkbaturu@gmail.com

ENV PYTHONUNBUFFERED 1

RUN apt-get update && apt-get upgrade -y
RUN apt-get install -y rabbitmq-server libsasl2-dev libsasl2-modules alien libaio1

ADD config/cppyy_wheel/cppyy_cling-6.14.2.1-py2.py3-none-manylinux1_x86_64.whl /home/analy/wheels/
RUN pip install /home/analy/wheels/cppyy_cling-6.14.2.1-py2.py3-none-manylinux1_x86_64.whl

ADD config/cppyy_wheel/cppyy_backend-1.4.2-py2.py3-none-manylinux1_x86_64.whl /home/analy/wheels/
RUN pip install /home/analy/wheels/cppyy_backend-1.4.2-py2.py3-none-manylinux1_x86_64.whl

ADD config/cppyy_wheel/CPyCppyy-1.3.6-cp36-cp36m-manylinux1_x86_64.whl /home/analy/wheels/
RUN pip install /home/analy/wheels/CPyCppyy-1.3.6-cp36-cp36m-manylinux1_x86_64.whl

ADD config/hosts /home/analy/hosts

ADD requirements.txt /home/analy/
RUN pip install -r /home/analy/requirements.txt

RUN git clone https://github.com/bitnine-oss/agensgraph-python.git /home/analy/agensgraph-python
RUN cd /home/analy/agensgraph-python && python setup.py install

ADD config/tensorflow_wheel/tensorflow-1.10.1-cp36-cp36m-linux_x86_64.whl /home/analy/wheels/
RUN pip install /home/analy/wheels/tensorflow-1.10.1-cp36-cp36m-linux_x86_64.whl

RUN pip install jupyter
ADD config/jupyter/jupyter_notebook_config.py /root/.jupyter/
RUN mkdir /root/.jupyter/custom
RUN echo '.CodeMirror pre {font-family: Inconsolata-g; font-size: 12pt; line-height: 140%;}\n.container { width:100% !important; }\ndiv.output pre{\n    font-family: Inconsolata-g;\n    font-size: 12pt;\n}' >> /root/.jupyter/custom/custom.css

RUN pip install django-silk

ADD config/celeryrestart.sh /usr/local/bin/
RUN chmod 755 /usr/local/bin/celeryrestart.sh

ADD config/oracle_instant_client/oracle-instantclient19.3-basic-19.3.0.0.0-1.x86_64.rpm /home/analy/oracle_instant_client/
ADD config/oracle_instant_client/oracle-instantclient19.3-devel-19.3.0.0.0-1.x86_64.rpm /home/analy/oracle_instant_client/
ADD config/oracle_instant_client/oracle-instantclient19.3-sqlplus-19.3.0.0.0-1.x86_64.rpm /home/analy/oracle_instant_client/

RUN alien -i /home/analy/oracle_instant_client/oracle-instantclient19.3-basic-19.3.0.0.0-1.x86_64.rpm
RUN alien -i /home/analy/oracle_instant_client/oracle-instantclient19.3-devel-19.3.0.0.0-1.x86_64.rpm
RUN alien -i /home/analy/oracle_instant_client/oracle-instantclient19.3-sqlplus-19.3.0.0.0-1.x86_64.rpm

ENV ORACLE_HOME /usr/lib/oracle/19.3/client64
ENV LD_LIBRARY_PATH $ORACLE_HOME/lib
ENV PATH $ORACLE_HOME/bin:$PATH

RUN ln -s $LD_LIBRARY_PATH/libocci.so.19.1 $LD_LIBRARY_PATH/libocci.so

WORKDIR /home/analy/analysystem