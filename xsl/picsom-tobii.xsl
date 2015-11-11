<?xml version="1.0" encoding="utf-8"?>

<!-- $Id: picsom-tobii.xsl,v 1.9 2010/12/02 14:24:43 jorma Exp $ -->

<xsl:stylesheet 
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:fo="http://www.w3.org/1999/XSL/Format"
  xmlns:picsom="http://www.cis.hut.fi/picsom/ns"
  version="1.0">

  <xsl:param name="tnwidth"   select="120"/>
  <xsl:param name="tnheight"  select="120"/>
  <xsl:param name="tncolumns" select="5"/>

  <xsl:variable name="useradioselection" select="0"/>

  <xsl:output method="html" indent="yes"/>

  <xsl:template match="/">
    <html>
      <head>
        <xsl:call-template name="meta"/>
        <title>PicSOM content-based multimedia retrieval</title>
        <script src="/picsom.js" type="text/javascript"/>
      </head>
      <body>
        <xsl:apply-templates/>
      </body>
    </html>
  </xsl:template>

  <xsl:template name="meta">
    <xsl:copy-of select="//meta"/>
    <meta name="DC.Title" content="PicSOM content-based image retrieval"/>
    <meta name="DC.Subject" content="content-based image retrieval"/>
    <meta name="AC.Email" content="picsom@cis.hut.fi"/>
    <xsl:for-each select="//picsom:result/picsom:refresh">
      <meta http-equiv="refresh">
        <xsl:attribute name="content"><xsl:value-of select="."/></xsl:attribute>
      </meta>
    </xsl:for-each>
  </xsl:template>

  <xsl:template match="//picsom:link">
    <xsl:element name="a">
      <xsl:attribute name="href">
        /<xsl:value-of select="@href"/></xsl:attribute>
      <xsl:value-of select="@href"/>
    </xsl:element><xsl:text>  </xsl:text>
    <xsl:value-of select="@text"/>
    <xsl:element name="br"/>
  </xsl:template>

  <xsl:template match="//picsom:get-databaselist">
    <xsl:element name="a">
      <xsl:attribute name="href">/databaselist</xsl:attribute>
      databaselist
    </xsl:element>
    <xsl:element name="br"/>
  </xsl:template>

  <xsl:template match="//picsom:get-querylist">
    <xsl:element name="a">
      <xsl:attribute name="href">/querylist</xsl:attribute>
      querylist
    </xsl:element>
    <xsl:element name="br"/>
  </xsl:template>

  <xsl:template match="//picsom:get-versions">
    <xsl:element name="a">
      <xsl:attribute name="href">/versions</xsl:attribute>
      versions
    </xsl:element>
    <xsl:element name="br"/>
  </xsl:template>

  <xsl:template match="//picsom:threadlist">
    <h2> List of existing threads </h2>
    <table>
      <thead style="font-weight:bold">
        <tr>
          <td>name</td>
          <td>pthread</td>
          <td>phase</td>
          <td>type</td>
          <td>text</td>
          <td>state</td>
          <td>slave_state</td>
        </tr>
      </thead>
      <tbody>
        <xsl:apply-templates/>
      </tbody>
    </table>
  </xsl:template>

  <xsl:template match="picsom:thread">
    <tr>
      <xsl:call-template name="threadinner"/>
    </tr>
  </xsl:template>

  <xsl:template name="threadinner">
    <td><xsl:value-of select="picsom:name"/></td>
    <td><xsl:value-of select="picsom:pthread"/></td>
    <td><xsl:value-of select="picsom:phase"/></td>
    <td><xsl:value-of select="picsom:type"/></td>
    <td><xsl:value-of select="picsom:text"/></td>
    <td><xsl:value-of select="picsom:state"/></td>
    <td><xsl:value-of select="picsom:slave_state"/></td>
  </xsl:template>

  <xsl:template match="//picsom:analysislist">
    <h2> List of existing analyses </h2>
    <table>
      <thead style="font-weight:bold">
        <tr>
          <td>identity</td>
          <td>memory</td>
          <td>name</td>
          <td>pthread</td>
          <td>phase</td>
          <td>type</td>
          <td>text</td>
          <td>state</td>
          <td>slave_state</td>
        </tr>
      </thead>
      <tbody>
        <xsl:apply-templates/>
      </tbody>
    </table>
  </xsl:template>

  <xsl:template match="//picsom:analysis">
    <tr>
      <td><xsl:value-of select="picsom:identity"/></td>
      <td><xsl:value-of select="picsom:memory"/></td>
      <xsl:for-each select="picsom:thread">
        <xsl:call-template name="threadinner"/>
      </xsl:for-each>
    </tr>
  </xsl:template>

  <xsl:template match="//picsom:databaselist">
    <h2> List of available databases </h2>
    <table border="1">
      <thead style="font-weight:bold">
        <tr>
          <td>name</td>
          <td>long name</td>
          <td>description</td>
          <td>size</td>
          <td>objectypes</td>
        </tr>
      </thead>
      <tbody>
        <xsl:apply-templates/>
      </tbody>
    </table>
  </xsl:template>

  <xsl:template match="//picsom:databaselist/picsom:database">
    <tr>
      <td>
        <xsl:element name="a">
          <xsl:attribute name="href">/database/<xsl:value-of select="picsom:name"/>
          </xsl:attribute>
          <xsl:value-of select="picsom:name"/>
        </xsl:element>
      </td>
      <td><xsl:value-of select="picsom:longname"/></td>
      <td>
        <span><xsl:value-of select="picsom:shorttext"/></span>
        <span style="display:none"><xsl:value-of select="picsom:longtext"/>
        </span>
        <form>
          <button type="button" onclick='toggle_short_long(this)'>MORE</button>
        </form>
        <xsl:variable name="context" select="picsom:context"/>
        <xsl:if test="$context='yes'"><xsl:element name="a">
        <xsl:attribute name="href"><xsl:value-of 
        select="picsom:name"/>/contextstate</xsl:attribute>
        contextstate</xsl:element></xsl:if>
      </td><xsl:text>
  </xsl:text>
      <td><xsl:value-of select="picsom:size"/></td>
      <td><xsl:value-of select="picsom:objecttypes"/></td>
    </tr>
  </xsl:template>

  <xsl:template match="//picsom:result/picsom:database">
  </xsl:template>

  <xsl:template match="//picsom:featurelist">
    <span id="featurelist"> 
    <form>
      <xsl:attribute name="method">post</xsl:attribute>
      <xsl:attribute name="action">/action</xsl:attribute>
      <xsl:attribute name="enctype">multipart/form-data</xsl:attribute>
      <xsl:attribute name="name">f</xsl:attribute>
      <xsl:attribute name="id">f_id</xsl:attribute>
      <xsl:call-template name="algorithmlistinner"/>
      <xsl:call-template name="featurelistinner"/>
      <p/>
      <input type="submit" name="start" value="Start query"/>
      <button type="button" name="restart" onclick="window.location.href='/'">Restart</button><xsl:text>
    </xsl:text>
    </form>
  </span>
  </xsl:template>

  <xsl:template name="featurelistinner">
    <h2> List of available features </h2>
    <xsl:for-each select="//picsom:featurelist">
      <xsl:copy/>
      <table>
        <thead style="font-weight:bold">
          <tr>
            <td></td>
            <td>longname</td>
            <td>shorttext</td>
            <td>size</td>
            <td>objecttype</td>
            <td>counts</td>
            <td>type</td>
          </tr>
        </thead>
        <xsl:apply-templates select="picsom:feature">
          <xsl:sort select="picsom:name"/>
        </xsl:apply-templates>
      </table>
    </xsl:for-each>
    <xsl:apply-templates select="picsom:featurealias"/>
  </xsl:template>

  <xsl:template match="//picsom:feature">
    <tr>
      <td>
        <input>
          <xsl:attribute name="type">checkbox</xsl:attribute>
          <xsl:attribute name="name">featsel</xsl:attribute>
          <xsl:attribute name="value"><xsl:value-of select="picsom:name"/>
          </xsl:attribute>
          <xsl:variable name="selected" select="picsom:selected"/>
          <xsl:if test="$selected=1"><xsl:attribute name="checked"/></xsl:if>
          <xsl:attribute name="id"><xsl:value-of select="picsom:name"/>
          </xsl:attribute>
        </input>
      </td>
      <td><xsl:value-of select="picsom:longname"/></td>
      <td><xsl:value-of select="picsom:shorttext"/></td>
      <td>
        <xsl:variable name="fsize" select="picsom:size"/>
        <xsl:choose>
          <xsl:when test="$fsize > 0"><xsl:value-of select="$fsize"/></xsl:when>
          <xsl:otherwise>-</xsl:otherwise>
        </xsl:choose>
      </td>
      <td><xsl:value-of select="picsom:objecttype"/></td>
      <td><xsl:value-of select="picsom:counts"/></td>
      <td><xsl:value-of select="picsom:type"/></td>
    </tr><xsl:text>
  </xsl:text>
  </xsl:template>

  <xsl:template match="//picsom:featurealias">
    <xsl:variable name="faname" select="@name"/>
    <xsl:variable name="fatext" select="@text"/>
    <xsl:variable name="favalue" select="@value"/>
    <button type="button" name="{$faname}" onclick='set_features("{$favalue}")'>
      <xsl:value-of select="$fatext"/></button><xsl:text>
  </xsl:text>
  </xsl:template>

  <xsl:template match="//picsom:algorithmlist"/>

  <xsl:template match="//picsom:algorithmlistXX">
    <span id="algorithmlist"> 
    <h2> List of available algorithms </h2>
    <form>
      <xsl:attribute name="method">post</xsl:attribute>
      <xsl:attribute name="action">/action</xsl:attribute>
      <xsl:attribute name="enctype">multipart/form-data</xsl:attribute>
      <xsl:attribute name="name">f</xsl:attribute>
      <xsl:attribute name="id">f_id</xsl:attribute>
      <table>
        <xsl:apply-templates/>
      </table>
      <input type="submit" name="start" value="Start query"/>
    </form>
  </span>
  </xsl:template>

  <xsl:template name="algorithmlistinner">
    <h2> List of available algorithms </h2>
    <xsl:for-each select="//picsom:algorithmlist">
      <xsl:copy/>
    <table>
      <thead style="font-weight:bold">
        <tr>
          <td></td>
          <td>name</td>
          <td>label</td>
          <td>description</td>
        </tr>
      </thead>
      <xsl:apply-templates/>
    </table>
    </xsl:for-each>
  </xsl:template>

  <xsl:template match="//picsom:algorithm">
    <tr>
      <td>
        <input>
          <xsl:attribute name="type">radio</xsl:attribute>
          <xsl:attribute name="name">algsel</xsl:attribute>
          <xsl:attribute name="value"><xsl:value-of select="picsom:name"/>
          </xsl:attribute>
          <xsl:variable name="default" select="picsom:default"/>
          <xsl:if test="$default=1"><xsl:attribute name="checked"/></xsl:if>
          <xsl:attribute name="id"><xsl:value-of select="picsom:name"/>
          </xsl:attribute>
        </input>
      </td>
      <td><xsl:value-of select="picsom:name"/></td>
      <td><xsl:value-of select="picsom:label"/></td>
      <td><xsl:value-of select="picsom:description"/></td>
    </tr>
    <xsl:apply-templates select="picsom:parameterlist"/>
  </xsl:template>

  <xsl:template match="//picsom:parameterlist">
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="//picsom:parameter">
    <tr>
      <td/>
      <td>
        <input>
          <xsl:attribute name="type">checkbox</xsl:attribute>
          <xsl:attribute name="name">paramsel</xsl:attribute>
          <xsl:attribute name="value"><xsl:value-of select="../../picsom:name"/>:<xsl:value-of select="@name"/>
          </xsl:attribute>
          <xsl:if test="@defval='true'"><xsl:attribute name="checked"/></xsl:if>
          <xsl:attribute name="id"><xsl:value-of select="picsom:name"/>
          </xsl:attribute>
        </input>
        <xsl:value-of select="@name"/>
      </td>
    </tr>
  </xsl:template>

  <xsl:template match="//picsom:connectionlist">
    <h2> List of existing connections </h2>
    <table>
      <thead style="font-weight:bold">
        <tr>
          <td>type</td>
          <td>protocol</td>
          <td>state</td>
          <td>port</td>
          <td>wfd</td>
          <td>rfd</td>
          <td>identity</td>
        </tr>
      </thead>
      <tbody>
        <xsl:apply-templates/>
      </tbody>
    </table>
  </xsl:template>

  <xsl:template match="//picsom:connection">
    <tr>
      <td><xsl:value-of select="picsom:type"/></td>
      <td><xsl:value-of select="picsom:protocol"/></td>
      <td><xsl:value-of select="picsom:state"/></td>
      <td><xsl:value-of select="picsom:port"/></td>
      <td><xsl:value-of select="picsom:wfd"/></td>
      <td><xsl:value-of select="picsom:rfd"/></td>
      <td><xsl:value-of select="picsom:identity"/></td>
    </tr>
  </xsl:template>

  <xsl:template match="//picsom:objects">
    <span id="objectlists" style="display:none"> 
    <form>
      <xsl:attribute name="method">post</xsl:attribute>
      <xsl:attribute name="action">/action</xsl:attribute>
      <xsl:attribute name="enctype">multipart/form-data</xsl:attribute>
      <xsl:attribute name="name">f</xsl:attribute>
      <xsl:attribute name="id">f_id</xsl:attribute>
      <xsl:apply-templates select="picsom:questionobjectlist"/>
      <!--
      <button type="button" name="selectall" onclick='select_all_checkboxes(this)'>Select all</button><xsl:text>
    </xsl:text>
      <button type="button" name="unselectall" onclick='unselect_all_checkboxes(this)'>Unselect all</button><xsl:text>
    </xsl:text>
    <button type="button" name="restart" onclick="window.location.href='/'">Restart</button>
    <xsl:variable name="resqlink">/database/<xsl:value-of select="//picsom:name"/></xsl:variable>
    <button type="button" name ="restartquery" onclick="window.location.href='{$resqlink}'">Restart query</button><br/><br/><xsl:text>
    </xsl:text>
    <input type="submit" name="continue" value="Continue query"/><xsl:text>
  </xsl:text> -->
   </form>
  </span>

  <script type="text/javascript">
    showimages();
  </script>    
  </xsl:template>

  <xsl:template match="//picsom:plusobjectlist">
    <form><b>Relevant marked images </b>(<span id="pluscount"></span>)
    <button type="button" onclick='toggle_table(this)'>hide</button>
    </form>
    <span>
    <table width="100%">
      <xsl:for-each select="picsom:objectinfo[position() mod $tncolumns = 1]">
        <tr>
          <xsl:apply-templates
            select=".|following-sibling::picsom:objectinfo[position() &lt; $tncolumns]"/>
        </tr>
      </xsl:for-each>
    </table>
  </span>
  <script type="text/javascript">
    set_nplusimages();
  </script>    
  <br/>
  </xsl:template>

  <xsl:template match="//picsom:minusobjectlist">
    <form><b>Non-relevant marked images </b>(<span id="minuscount"></span>)
    <button type="button" onclick='toggle_table(this)'>show</button>
  </form>
    <span style="display:none">
    <table width="100%">
      <xsl:for-each select="picsom:objectinfo[position() mod $tncolumns = 1]">
        <tr>
          <xsl:apply-templates
            select=".|following-sibling::picsom:objectinfo[position() &lt; $tncolumns]"/>
        </tr>
      </xsl:for-each>
    </table>
  </span>
  <script type="text/javascript">
    set_nminusimages();
  </script>    
  <br/>
  </xsl:template>

  <xsl:template match="//picsom:zeroobjectlist">
    <form><b>Neutral marked images </b>(<span id="zerocount"></span>)
    <button type="button" onclick='toggle_table(this)'>show</button>
  </form>
    <span style="display:none">
    <table width="100%">
      <xsl:for-each select="picsom:objectinfo[position() mod $tncolumns = 1]">
        <tr>
          <xsl:apply-templates
            select=".|following-sibling::picsom:objectinfo[position() &lt; $tncolumns]"/>
        </tr>
      </xsl:for-each>
    </table>
  </span>
  <script type="text/javascript">
    set_nzeroimages();
  </script>    
  <br/>
  </xsl:template>

  <xsl:template match="//picsom:showobjectlist">
    <form><b>Show images </b>(<span id="showcount"></span>)
    <button type="button" onclick='toggle_table(this)'>show</button>
  </form>
    <span style="display:none">
    <table width="100%">
      <xsl:for-each select="picsom:objectinfo[position() mod $tncolumns = 1]">
        <tr>
          <xsl:apply-templates
            select=".|following-sibling::picsom:objectinfo[position() &lt; $tncolumns]"/>
        </tr>
      </xsl:for-each>
    </table>
  </span>
  <script type="text/javascript">
    set_nshowimages();
  </script>    
  <br/>
  </xsl:template>

  <xsl:template match="//picsom:questionobjectlist">
    <span>
    <table width="100%">
      <xsl:for-each select="picsom:objectinfo[position() mod $tncolumns = 1]">
        <tr>
          <xsl:apply-templates
            select=".|following-sibling::picsom:objectinfo[position() &lt; $tncolumns]"/>
        </tr>
      </xsl:for-each>
    </table>
  </span>
  <script type="text/javascript">
    set_nquestionimages();
  </script>    
  <br/>
  </xsl:template>

  <xsl:template match="//picsom:objectinfo">
    <!-- <h3> Image </h3> -->
    <!--       <td width="100%" height="100%" align="center" valign="center"> -->
    <xsl:text>
    </xsl:text>
       <td>
         <xsl:choose>
           <xsl:when test="$useradioselection=1"> 
             <xsl:call-template name="radioimage"/>
           </xsl:when>
           <xsl:otherwise>
             <xsl:call-template name="checkboximage"/>
           </xsl:otherwise>
         </xsl:choose>
      </td>
        <xsl:if test="(position() = last()) and (position() &lt; $tncolumns)">
          <xsl:call-template name="fillercells">
            <xsl:with-param name="cellCount" select="$tncolumns - position()"/>
          </xsl:call-template>
        </xsl:if>
  </xsl:template>

  <xsl:template name="fillercells">
    <xsl:param name="cellCount"/>
    <td>&#160;</td>
    <xsl:if test="$cellCount > 1">
      <xsl:call-template name="fillercells">
        <xsl:with-param name="cellCount" select="$cellCount - 1"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>
  
  <xsl:template name="checkboximage">
    <!-- <input>
      <xsl:attribute name="type">checkbox</xsl:attribute>
      <xsl:call-template name="imageinput"/>
    </input> -->
    <span>
      <xsl:attribute name="name">obj-picsom:questionobjectlist</xsl:attribute>
    </span>
    <xsl:call-template name="visibleimage"/>
  </xsl:template>

  <xsl:template name="radioimage">
    <input>
      <xsl:attribute name="type">radio</xsl:attribute>
      <xsl:call-template name="imageinput"/>
    </input>
    <xsl:call-template name="visibleimage"/>
  </xsl:template>

  <xsl:template name="imageinput">
    <xsl:attribute name="name">obj-<xsl:value-of 
    select="name(..)"/></xsl:attribute>
    <xsl:attribute name="class"><xsl:value-of select="name(..)"/></xsl:attribute>
    <xsl:attribute name="value"><xsl:value-of select="picsom:label"/>,</xsl:attribute>
  <xsl:variable name="selected" select="picsom:aspectlist/picsom:aspect[@name='']/@value"/>
  <xsl:if test="$selected=1"><xsl:attribute name="checked"/></xsl:if>
    <xsl:attribute name="id">
      <xsl:value-of select="picsom:objectfileinfo/picsom:name"/>
  </xsl:attribute>
  </xsl:template>

  <xsl:template name="clickableimage">
    <xsl:element name="a">
      <xsl:attribute name="href">/click/<xsl:value-of 
        select="picsom:database"/>/<xsl:value-of select="picsom:label"/>
      </xsl:attribute>
      <xsl:call-template name="visibleimage"/>
    </xsl:element>
  </xsl:template>

  <xsl:template name="visibleimage">
    <table><tr>
    <xsl:element name="td">
    <xsl:attribute name="style">height: <xsl:number value="$tnheight"/>px; 
    width: <xsl:number  value="$tnwidth"/>px;</xsl:attribute>
    <xsl:element name="img">
      <xsl:variable name="selected" 
        select="picsom:aspectlist/picsom:aspect[@name='']/@value"/>
      <xsl:variable name="bordercolor">
        <xsl:choose>
          <xsl:when test="$selected=1">red</xsl:when>
          <xsl:otherwise>white</xsl:otherwise>
        </xsl:choose>
      </xsl:variable> 
      <xsl:attribute name="style">
        border:3px dashed <xsl:value-of select="$bordercolor"/>
      </xsl:attribute>
      <xsl:attribute name="src">
        /database/<xsl:value-of select="picsom:database"/>/<xsl:value-of
        select="picsom:label"/>?tn
      </xsl:attribute>
      <xsl:attribute name="id">obj<xsl:value-of select="picsom:label"/>
      </xsl:attribute>
      <xsl:variable name="score" select="picsom:score"/>
      <xsl:choose>
        <xsl:when test="$score > 0"> 
          <xsl:attribute name="title">Label: <xsl:value-of 
            select="picsom:label"/>, score: <xsl:value-of 
            select="picsom:score"/>
          </xsl:attribute>
        </xsl:when>
        <xsl:otherwise>
          <xsl:attribute name="title">Label: <xsl:value-of
            select="picsom:label"/>
          </xsl:attribute>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:variable name="width"  select="picsom:frameinfo/picsom:width"/>
      <xsl:variable name="height" select="picsom:frameinfo/picsom:height"/>
      <xsl:choose>
        <xsl:when test="$tnheight*$width &gt; $tnwidth*$height">
	  <xsl:variable  name="seth" select="round($height div $width * $tnwidth)"/>
          <xsl:attribute name="width"><xsl:number  value="$tnwidth"/></xsl:attribute>
          <xsl:attribute name="height"><xsl:number value="$seth"/></xsl:attribute>
        </xsl:when>
        <xsl:otherwise>
	  <xsl:variable  name="setw" select="round($width div $height * $tnheight)"/>
          <xsl:attribute name="height"><xsl:number value="$tnheight"/></xsl:attribute>
          <xsl:attribute name="width"><xsl:number  value="$setw"/></xsl:attribute>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:element>
    </xsl:element>
    </tr>
    <tr><xsl:element name="td">
	<xsl:attribute name="id">txt<xsl:value-of select="picsom:label"/>
	</xsl:attribute>
	<xsl:attribute name="style">height:30px;width:400px</xsl:attribute>
	<xsl:value-of select="picsom:erfdata/picsom:belowtext"/>
    </xsl:element></tr>
    </table>
  </xsl:template>

  <xsl:template match="picsom:contextstate">
    <h2> Context state variables </h2>
    <table border="1">
      <thead style="font-weight:bold">
        <tr>
          <td>name</td>
          <td>value</td>
          <td>time</td>
        </tr>
      </thead>
      <tbody>
        <xsl:apply-templates/>
      </tbody>
    </table>
  </xsl:template>

  <xsl:template match="picsom:contextstate/picsom:variable">
    <tr>
      <td><xsl:value-of select="@name"/></td>
      <td><xsl:value-of select="@value"/></td>
      <td><xsl:value-of select="@time"/></td>
    </tr>
  </xsl:template>

  <xsl:template match="picsom:contentitemlist"/>

  <xsl:template match="picsom:contentitem"/>

  <xsl:template match="picsom:convtype"/>

  <xsl:template match="picsom:variables"/>

  <xsl:template match="picsom:parent_identity"/>

  <xsl:template match="picsom:xhtml"/>

  <xsl:template match="picsom:refresh"/>

  <xsl:template match="picsom:ajaxdata"/>

  <xsl:template match="picsom:childlist"/>

  <xsl:template match="//picsom:inrun"/>

  <xsl:template match="//picsom:versions">
    <h3> Versions </h3>
    <table>
      <thead style="font-weight:bold">
        <tr>
          <td>name</td>
          <td>version</td>
        </tr>
      </thead>
      <tbody>
        <xsl:apply-templates/>
      </tbody>
    </table>
  </xsl:template>

  <xsl:template match="//picsom:piece">
    <tr>
      <td><xsl:value-of select="@name"/></td>
      <td><xsl:value-of select="@version"/></td>
    </tr>
  </xsl:template>

</xsl:stylesheet>
